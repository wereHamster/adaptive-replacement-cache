
#include <arc.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define MAX(a, b) ( (a) > (b) ? (a) : (b) )
#define MIN(a, b) ( (a) < (b) ? (a) : (b) )

extern unsigned char objname(struct __arc_object *entry);

static char *listname(struct __arc *s, struct __arc_state *l)
{
    if (l == &s->mrug)
        return "mrug";
    else if (l == &s->mru)
        return "mru";
    if (l == &s->mfu)
        return "mfu";
    else if (l == &s->mfug)
        return "mfug";

    return "unknown";
}

void __arc_object_init(struct __arc_object *obj, unsigned long size)
{
    __list_init(&obj->head);
    obj->size = size;
}

static void __arc_balance(struct __arc *cache, unsigned long size);

/* Move the object to the given state. If the state transition requires,
* fetch, evict or destroy the object. */
static int __arc_move(struct __arc *cache, struct __arc_object *obj, struct __arc_state *state)
{
    printf("moving %02x from %s to %s, %d\n", objname(obj), listname(cache, obj->state),
        listname(cache, state), obj->state ? obj->state->size : 0);

    if (obj->state) {
        obj->state->size -= obj->size;
        __list_remove(&obj->head);
    }

    if (state == NULL) {
        /* The object is being removed from the cache, destroy it. */
        cache->ops->destroy(obj);
    } else {
        if (state == &cache->mrug || state == &cache->mfug) {
            /* The object is being moved to one of the ghost lists, evict
             * the object from the cache. */
            cache->ops->evict(obj);
        } else if (obj->state != &cache->mru && obj->state != &cache->mfu) {
            /* The object is being moved from one of the ghost lists into
             * the MRU or MFU list, fetch the object into the cache. */
            __arc_balance(cache, obj->size);
            if (cache->ops->fetch(obj)) {
                /* If the fetch fails, put the object back to the list
                 * it was in before. */
                obj->state->size += obj->size;
                __list_prepend(&obj->head, &obj->state->head);
                
                return -1;
            }
        }

        __list_prepend(&obj->head, &state->head);

        obj->state = state;
        obj->state->size += obj->size;
    }
    
    return 0;
}

/* Return the LRU element from the given state. */
static struct __arc_object *__arc_state_tail(struct __arc_state *state)
{
    struct __list *head = state->head.prev;
    return __list_entry(head, struct __arc_object, head);
}

/* Balance the lists so that we can fit an object with the given size into
 * the cache. */
static void __arc_balance(struct __arc *cache, unsigned long size)
{
    /* First move objects from MRU/MFU to their respective ghost lists. */
    while (cache->mru.size + cache->mfu.size + size > cache->c) {
        printf("mru: %lu, mfu: %lu\n", cache->mru.size, cache->mfu.size);
        
        if (cache->mru.size > cache->p) {
            struct __arc_object *obj = __arc_state_tail(&cache->mru);
            __arc_move(cache, obj, &cache->mrug);
        } else if (cache->mfu.size > 0) {
            struct __arc_object *obj = __arc_state_tail(&cache->mfu);
            __arc_move(cache, obj, &cache->mfug);
        } else {
            break;
        }
    }
    
    /* Then start removing objects from the ghost lists. */
    while (cache->mrug.size + cache->mfug.size > cache->c) {
        printf("mrug: %lu, mfug: %lu\n", cache->mrug.size, cache->mfug.size);
        
        if (cache->mfug.size > cache->p) {
            struct __arc_object *obj = __arc_state_tail(&cache->mfug);
            __arc_move(cache, obj, NULL);
        } else if (cache->mrug.size > 0) {
            struct __arc_object *obj = __arc_state_tail(&cache->mrug);
            __arc_move(cache, obj, NULL);
        } else {
            break;
        }
    }
}


/* Create a new cache. */
struct __arc *__arc_create(struct __arc_ops *ops, unsigned long c)
{
    struct __arc *state = malloc(sizeof(struct __arc));
    memset(state, 0, sizeof(struct __arc));

    state->ops = ops;

    state->c = c;
    state->p = c >> 1;

    __list_init(&state->mrug.head);
    __list_init(&state->mru.head);
    __list_init(&state->mfu.head);
    __list_init(&state->mfug.head);

    return state;
}

/* Destroy the given cache. Free all objects which remain in the cache. */
void __arc_destroy(struct __arc *cache)
{
    struct __list *iter;
    
    __list_each(iter, &cache->mrug.head) {
        struct __arc_object *obj = __list_entry(iter, struct __arc_object, head);
        cache->ops->destroy(obj);
    }
    __list_each(iter, &cache->mru.head) {
        struct __arc_object *obj = __list_entry(iter, struct __arc_object, head);
        cache->ops->destroy(obj);
    }
    __list_each(iter, &cache->mfu.head) {
        struct __arc_object *obj = __list_entry(iter, struct __arc_object, head);
        cache->ops->destroy(obj);
    }
    __list_each(iter, &cache->mfug.head) {
        struct __arc_object *obj = __list_entry(iter, struct __arc_object, head);
        cache->ops->destroy(obj);
    }

    free(cache);
}

static struct __arc_object *__search_in(struct __arc *state, struct __list *list, const void *key)
{
    struct __list *pos;
    __list_each(pos, list) {
        struct __arc_object *e = __list_entry(pos, struct __arc_object, head);
        if (state->ops->cmp(e, key) == 0)
            return e;
    }

    return NULL;
}

static struct __arc_object *__search(struct __arc *state, const void *key)
{
    struct __arc_object *e;

    e = __search_in(state, &state->mru.head, key);
    if (e) {
        return e;
    }

    e = __search_in(state, &state->mfu.head, key);
    if (e) {
        return e;
    }

    e = __search_in(state, &state->mrug.head, key);
    if (e) {
        return e;
    }

    e = __search_in(state, &state->mfug.head, key);
    if (e) {
        return e;
    }

    return NULL;
}

/* Lookup an object with the given key. */
struct __arc_object *__arc_lookup(struct __arc *cache, const void *key)
{
    struct __arc_object *obj = __search(cache, key);

    if (obj) {
        if (obj->state == &cache->mru || obj->state == &cache->mfu) {
            /* Object is already in the cache, move it to the head of the
             * MFU list. This operation can not fail since the data is
             * already in the cache. */
            __arc_move(cache, obj, &cache->mfu);
        } else if (obj->state == &cache->mrug) {
            cache->p = MIN(cache->c, cache->p + MAX(cache->mfug.size / cache->mrug.size, 1));
            if (__arc_move(cache, obj, &cache->mfu)) {
                return NULL;
            }
        } else if (obj->state == &cache->mfug) {
            cache->p = MIN(cache->c, MAX(0, cache->p - MAX(cache->mrug.size / cache->mfug.size, 1)));
            if (__arc_move(cache, obj, &cache->mfu)) {
                return NULL;
            }
        } else {
            assert(0);
        }
    } else {
        obj = cache->ops->alloc(key);
        if (!obj)
            return NULL;

        if (cache->mru.size + cache->mrug.size == cache->c) {
            if (cache->mru.size < cache->c) {
                struct __arc_object *e = __arc_state_tail(&cache->mrug);
                __arc_move(cache, e, NULL);
            } else {                
                struct __arc_object *e = __arc_state_tail(&cache->mru);
                __arc_move(cache, e, NULL);
            }
        } else if (cache->mru.size < cache->c && cache->mru.size + cache->mfu.size >= cache->c) {
            if (cache->mru.size + cache->mfu.size == cache->c * 2) {                
                struct __arc_object *e = __arc_state_tail(&cache->mfug);
                __arc_move(cache, e, NULL);
            }
        }

        __arc_move(cache, obj, &cache->mru);
    }
    
    printf("%lu - %lu - %lu - %lu\n", cache->mrug.size, cache->mru.size, cache->mfu.size, cache->mfug.size);

    return obj;
}
