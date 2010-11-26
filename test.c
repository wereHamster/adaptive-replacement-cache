
#include <arc.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

/* This is the object we're managing. It has a name (sha1)
* and some data. This data will be loaded when ARC instruct
* us to do so. */
struct object {
	unsigned char sha1[20];
	struct __arc_object entry;

	void *data;
};

unsigned char objname(struct __arc_object *entry)
{
	struct object *obj = __arc_list_entry(entry, struct object, entry);
	return obj->sha1[0];
}


/**
* Here are the operations implemented
*/

static unsigned long __op_hash(const void *key)
{
	const unsigned char *sha1 = key;
	return sha1[0];
}

static int __op_compare(struct __arc_object *e, const void *key)
{
	struct object *obj = __arc_list_entry(e, struct object, entry);
	return memcmp(obj->sha1, key, 20);
}

static struct __arc_object *__op_create(const void *key)
{
	struct object *obj = malloc(sizeof(struct object));

	memcpy(obj->sha1, key, 20);
	obj->data = NULL;

	__arc_object_init(&obj->entry, rand() % 100);
	return &obj->entry;
}

static int __op_fetch(struct __arc_object *e)
{
	struct object *obj = __arc_list_entry(e, struct object, entry);
	obj->data = malloc(200);
	return 0;
}

static void __op_evict(struct __arc_object *e)
{
	struct object *obj = __arc_list_entry(e, struct object, entry);
	free(obj->data);
}

static void __op_destroy(struct __arc_object *e)
{
	struct object *obj = __arc_list_entry(e, struct object, entry);
	free(obj);
}

static struct __arc_ops ops = {
	.hash		= __op_hash,
	.cmp		= __op_compare,
	.create		= __op_create,
	.fetch		= __op_fetch,
	.evict		= __op_evict,
	.destroy	= __op_destroy
};

static void stats(struct __arc *s)
{
	int i = 0;
	struct __arc_list *pos;

	__arc_list_each_prev(pos, &s->mrug.head) {
		struct __arc_object *e = __arc_list_entry(pos, struct __arc_object, head);
		assert(e->state == &s->mrug);
		struct object *obj = __arc_list_entry(e, struct object, entry);
		printf("[%02x]", obj->sha1[0]);
	}
	printf(" + ");
	__arc_list_each_prev(pos, &s->mru.head) {
		struct __arc_object *e = __arc_list_entry(pos, struct __arc_object, head);
		assert(e->state == &s->mru);
		struct object *obj = __arc_list_entry(e, struct object, entry);
		printf("[%02x]", obj->sha1[0]);

		if (i++ == s->p)
			printf(" # ");
	}
	printf(" + ");
	__arc_list_each(pos, &s->mfu.head) {
		struct __arc_object *e = __arc_list_entry(pos, struct __arc_object, head);
		assert(e->state == &s->mfu);
		struct object *obj = __arc_list_entry(e, struct object, entry);
		printf("[%02x]", obj->sha1[0]);

		if (i++ == s->p)
			printf(" # ");
	}
	if (i == s->p)
		printf(" # ");
	printf(" + ");
	__arc_list_each(pos, &s->mfug.head) {
		struct __arc_object *e = __arc_list_entry(pos, struct __arc_object, head);
		assert(e->state == &s->mfug);
		struct object *obj = __arc_list_entry(e, struct object, entry);
		printf("[%02x]", obj->sha1[0]);
	}
	printf("\n");
}

#define MAXOBJ 16

int main(int argc, char *argv[])
{
	srandom(time(NULL));

	unsigned char sha1[MAXOBJ][20];
	for (int i = 0; i < MAXOBJ; ++i) {
		memset(sha1[i], 0, 20);
		sha1[i][0] = i;
	}

	struct __arc *s = __arc_create(&ops, 300);

	for (int i = 0; i < 4 * MAXOBJ; ++i) {
		unsigned char *cur = sha1[random() & (MAXOBJ - 1)];
		printf("get %02x: ", cur[0]);
		assert(__arc_lookup(s, cur));
		stats(s);
	}

	for (int i = 0; i < MAXOBJ; ++i) {
		unsigned char *cur = sha1[random() & (MAXOBJ / 4 - 1)];
		printf("get %02x: ", cur[0]);
		assert(__arc_lookup(s, cur));
		stats(s);
	}

	for (int i = 0; i < 4 * MAXOBJ; ++i) {
		unsigned char *cur = sha1[random() & (MAXOBJ - 1)];
		printf("get %02x: ", cur[0]);
		assert(__arc_lookup(s, cur));
		stats(s);
	}

	return 0;
}
