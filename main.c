
#include <arc.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

struct object {
    unsigned char sha1[20];
    struct __arc_object entry;

    void *data;
};

unsigned char objname(struct __arc_object *entry)
{
    struct object *obj = __list_entry(entry, struct object, entry);
    return obj->sha1[0];
}

static int __ops_compare(struct __arc_object *e, const void *key)
{
    struct object *obj = __list_entry(e, struct object, entry);
  //  printf("compare %02x - %02x\n", sha1[0], obj->sha1[0]);
    return memcmp(obj->sha1, key, 20);
}

static struct __arc_object *__ops_alloc(const void *key)
{
    struct object *obj = malloc(sizeof(struct object));
    memset(obj, 0, sizeof(struct object));

    __list_init(&obj->entry.head);

    memcpy(obj->sha1, key, 20);
    obj->data = NULL;

    printf("alloc: %02x\n", objname(&obj->entry));

    return &obj->entry;
}

static unsigned long __ops_fetch(struct __arc_object *e)
{
    struct object *obj = __list_entry(e, struct object, entry);
    obj->data = malloc(200);

    printf("fetch: %02x\n", objname(&obj->entry));
    return rand() % 100;
}

static void __ops_evict(struct __arc_object *e)
{
    struct object *obj = __list_entry(e, struct object, entry);
    free(obj->data);

    printf("evict: %02x\n", objname(&obj->entry));
}

static void __ops_destroy(struct __arc_object *e)
{
    struct object *obj = __list_entry(e, struct object, entry);
    printf("free: %02x\n", objname(&obj->entry));
    free(obj);
}

static struct __arc_ops ops = {
    .cmp = __ops_compare,
    .alloc = __ops_alloc,
    .fetch = __ops_fetch,
    .evict = __ops_evict,
    .destroy = __ops_destroy
};

static void stats(struct __arc *s)
{
    struct __list *pos;

    printf("+");
    __list_each_prev(pos, &s->mrug.head) {
        struct __arc_object *e = __list_entry(pos, struct __arc_object, head);
        assert(e->state == &s->mrug);
        struct object *obj = __list_entry(e, struct object, entry);
        printf("[%02x]", obj->sha1[0]);
    }
    printf("+");
    int i = 0;
    __list_each_prev(pos, &s->mru.head) {
        struct __arc_object *e = __list_entry(pos, struct __arc_object, head);
        assert(e->state == &s->mru);
        struct object *obj = __list_entry(e, struct object, entry);
        printf("[%02x]", obj->sha1[0]);

        if (i++ == s->p)
            printf("#");
    }
    printf("+");
    __list_each(pos, &s->mfu.head) {
        struct __arc_object *e = __list_entry(pos, struct __arc_object, head);
        assert(e->state == &s->mfu);
        struct object *obj = __list_entry(e, struct object, entry);
        printf("[%02x]", obj->sha1[0]);

        if (i++ == s->p)
            printf("#");
    }
    if (i == s->p)
        printf("#");
    printf("+");
    __list_each(pos, &s->mfug.head) {
        struct __arc_object *e = __list_entry(pos, struct __arc_object, head);
        assert(e->state == &s->mfug);
        struct object *obj = __list_entry(e, struct object, entry);
        printf("[%02x]", obj->sha1[0]);
    }
    printf("+\n");
}

#define MAXOBJ 16

int main(int argc, char *argv[])
{
    struct __arc *s = __arc_create(&ops, 300);
    time_t now = time(NULL); //1275232080
    printf("seed: %lu\n", now);
    srandom(now);

    unsigned char sha1[MAXOBJ][20];
    for (int i = 0; i < MAXOBJ; ++i) {
        memset(sha1[i], 0, 20);
        sha1[i][0] = i;
    }

    for (int i = 0; i < 4 * MAXOBJ; ++i) {
        unsigned char *cur = sha1[random() & (MAXOBJ - 1)];
        printf("get %02x\n", cur[0]);
        struct __arc_object *e = __arc_lookup(s, cur);
        stats(s);
    }

    return 0;
}
