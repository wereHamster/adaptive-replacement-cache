
#ifndef __ARC_H__
#define __ARC_H__

#include <list.h>


struct __arc_l {
  unsigned long size;
  struct __list head;
};

struct __arc_e {
  struct __arc_l *list;
  struct __list head;
};

struct object {
  unsigned char sha1[20];
  struct __arc_e entry;

  void *data;
};

typedef int (*__arc_compare_t)(struct __arc_e *e, const void *key);
typedef struct __arc_e *(*__arc_alloc_t)(const void *key);
typedef int (*__arc_fetch_t)(struct __arc_e *e);
typedef void (*__arc_evict_t)(struct __arc_e *e);

struct __arc_ops {
  __arc_compare_t cmp;
  __arc_alloc_t alloc;
  __arc_fetch_t fetch;
  __arc_evict_t evict;
};

struct __arc_s {
  struct __arc_ops *ops;

  unsigned long c, p;
  struct __arc_l mrug, mru, mfu, mfug;
};

struct __arc_s *__arc_create(struct __arc_ops *ops, unsigned long c);
void __arc_destroy(struct __arc_s *s);

struct __arc_e *__arc_lookup(struct __arc_s *s, const void *key);
//void __arc_insert(struct __arc_s *s, struct __arc_e *e);
//void __arc_access(struct __arc_s *s, struct __arc_e *e);

#endif /* __ARC_H__ */
