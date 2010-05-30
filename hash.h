
#ifndef __HASH_H__
#define __HASH_H__

#include <list.h>

struct __arc_hash {
    unsigned long size;
    struct __arc_list *buckets;
};

static inline void __arc_hash_init(struct __arc_hash *hash) {
    hash->size = 3079;
    hash->buckets = malloc(hash->size * sizeof(struct __arc_list));
}

static inline void __arc_hash_insert(struct __arc_hash *hash, struct __arc_list *obj) {
    
}

#endif /* __HASH_H__ */
