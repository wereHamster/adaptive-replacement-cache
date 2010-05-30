
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
};

struct __arc_ops {
  int (*cmp) (struct __arc_object *obj, const void *key);
  
  struct __arc_object *(*alloc) (const void *key);
  int (*fetch) (struct __arc_object *obj);
  void (*evict) (struct __arc_object *obj);
  void (*destroy) (struct __arc_object *obj);
};

struct __arc {
  struct __arc_ops *ops;

  unsigned long c, p;
  struct __arc_state mrug, mru, mfu, mfug;
};

/* Functions to create and destroy the ARC */
struct __arc *__arc_create(struct __arc_ops *ops, unsigned long c);
void __arc_destroy(struct __arc *cache);

/* Lookup an object in the cache. The ARC automatically creates and
 * loads the object if it does not exists yet. */
struct __arc_object *__arc_lookup(struct __arc *cache, const void *key);


#endif /* __ARC_H__ */
