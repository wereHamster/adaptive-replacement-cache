
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

/* Move the object to the given state. If the state transition requires,
* fetch, evict or destroy the object. */
    static void __arc_move(struct __arc *cache, struct __arc_object *obj, struct __arc_state *state)
{
    printf("moving %02x from %s to %s, %d\n", objname(obj), listname(cache, obj->state),
        listname(cache, state), obj->state ? obj->state->size : 0);

    if (obj->state) {
        obj->state->size -= 1;
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
            /* The object is either new or being moved from one of the
             * ghost lists into the MRU or MFU list, fetch the object into
             * the cache. */
            cache->ops->fetch(obj);
        }

        __list_prepend(&obj->head, &state->head);

        obj->state = state;
        obj->state->size += 1;
        printf("new size: %d\n", obj->state->size);
    }
}

static struct __arc_object *__arc_state_tail(struct __arc_state *list)
{
    struct __list *head = list->head.prev;
    return __list_entry(head, struct __arc_object, head);
}

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

static struct __arc_state *__arc_replace(struct __arc *cache, struct __arc_state *list)
{
    if (cache->mru.size >= 1 && ((list == &cache->mfug && cache->mru.size == cache->p) || (cache->mru.size > cache->p))) {
        struct __arc_object *e = __arc_state_tail(&cache->mru);
        __arc_move(cache, e, &cache->mrug);

        return &cache->mrug;
    } else {
        struct __arc_object *e = __arc_state_tail(&cache->mfu);
        __arc_move(cache, e, &cache->mfug);

        return &cache->mfug;
    }
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

struct __arc_object *__arc_lookup(struct __arc *cache, const void *key)
{
    struct __arc_object *obj = __search(cache, key);

    if (obj) {
        if (obj->state == &cache->mru || obj->state == &cache->mfu) {
            /* Object is already in the cache, move it to the head of the MFU list */
            printf("mru/mfu hit\n");
            __arc_move(cache, obj, &cache->mfu);
        } else if (obj->state == &cache->mrug) {
            printf("mrug hit\n");
            if (cache->ops->fetch(obj))
                return NULL;

            cache->p = MIN(cache->c, cache->p + MAX(cache->mfug.size / cache->mrug.size, 1));
            __arc_replace(cache, obj->state);
            __arc_move(cache, obj, &cache->mfu);
        } else if (obj->state == &cache->mfug) {
            printf("mfug hit\n");
            if (cache->ops->fetch(obj))
                return NULL;

            cache->p = MAX(0, cache->p - MAX(cache->mrug.size / cache->mfug.size, 1));
            __arc_replace(cache, obj->state);
            __arc_move(cache, obj, &cache->mfu);
        } else {
            assert(0);
        }
    } else {
        obj = cache->ops->alloc(key);
        if (!obj)
            return NULL;

        if (cache->mru.size + cache->mrug.size == cache->c) {
            if (cache->mru.size < cache->c) {
                printf("miss 1\n");
                struct __arc_object *e = __arc_state_tail(&cache->mrug);
                __arc_move(cache, e, NULL);
                __arc_replace(cache, NULL);
            } else {
                printf("miss 2\n");
                
                struct __arc_object *e = __arc_state_tail(&cache->mru);
                cache->ops->destroy(e);
            }
        } else if (cache->mru.size < cache->c && cache->mru.size + cache->mfu.size >= cache->c) {
            if (cache->mru.size + cache->mfu.size == cache->c * 2) {
                printf("miss 3\n");
                
                struct __arc_object *e = __arc_state_tail(&cache->mfug);
                __arc_move(cache, e, NULL);
            } else {
                printf("miss 4\n");
                
            }
            __arc_replace(cache, NULL);
        }


        __arc_move(cache, obj, &cache->mru);
    }

    return obj;
}
