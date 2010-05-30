
#ifndef __ARC_H__
#define __ARC_H__

#include <list.h>

struct __arc_state {
    unsigned long size;
    struct __list head;
};

struct __arc_object {
    struct __arc_state *state;
    struct __list head;
    unsigned long size;
};

struct __arc_ops {
    /* Compare the object with the key. */
    int (*cmp) (struct __arc_object *obj, const void *key);

    /* Allocate a new object. */
    struct __arc_object *(*alloc) (const void *key);
    
    /* Fetch the data associated with the object. */
    unsigned long (*fetch) (struct __arc_object *obj);
    
    /* This function is called when the cache is full and we need to evict
     * objects from the cache. Free all data associated with the object. */
    void (*evict) (struct __arc_object *obj);
    
    /* This function is called when the object is completely removed from
     * the cache. Free the object data and the object itself. */
    void (*destroy) (struct __arc_object *obj);
};

struct __arc {
    struct __arc_ops *ops;

    unsigned long c, p;
    struct __arc_state mrug, mru, mfu, mfug;
};

/* Functions to create and destroy the cache. */
struct __arc *__arc_create(struct __arc_ops *ops, unsigned long c);
void __arc_destroy(struct __arc *cache);

/* Initialize a new object. */
void __arc_object_init(struct __arc_object *obj, unsigned long size);

/* Lookup an object in the cache. The cache automatically creates and
 * fetches the object if it does not exists yet. */
struct __arc_object *__arc_lookup(struct __arc *cache, const void *key);


#endif /* __ARC_H__ */
