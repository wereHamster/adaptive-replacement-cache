
#include <arc.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX(a, b) ( (a) > (b) ? (a) : (b) )

static unsigned char objname(struct __arc_e *entry)
{
  struct object *obj = __list_entry(entry, struct object, entry);
  return obj->sha1[0];
}

static char *listname(struct __arc_s *s, struct __arc_l *l)
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

static struct __arc_s *gs;

static void __arc_list_move(struct __arc_e *entry, struct __arc_l *list)
{
  printf("moving %02x from %s to %s, %d -> ", objname(entry), listname(gs, entry->list), listname(gs, list), entry->list ? entry->list->size : 0);

  if (entry->list) {
    entry->list->size -= 1;
    entry->list = 0;

    __list_remove(&entry->head);
    __list_init(&entry->head);
  }

  if (list) {
    __list_prepend(&entry->head, &list->head);

    entry->list = list;
    entry->list->size += 1;
  }

  printf("%d\n", entry->list ? entry->list->size : 0);
}

static struct __arc_e *__arc_list_remove(struct __arc_l *list)
{
  struct __list *head = list->head.prev;
  __list_remove(head);
  __list_init(head);
  return __list_entry(head, struct __arc_e, head);
}

struct __arc_s *__arc_create(struct __arc_ops *ops, unsigned long c)
{
  struct __arc_s *state = malloc(sizeof(struct __arc_s));
  memset(state, 0, sizeof(struct __arc_s));
  
  state->ops = ops;

  state->c = c;
  state->p = c >> 1;

  __list_init(&state->mrug.head);
  __list_init(&state->mru.head);
  __list_init(&state->mfu.head);
  __list_init(&state->mfug.head);

  gs = state;

  return state;
}

void __arc_destroy(struct __arc_s *state)
{
  free(state);
}

static struct __arc_l *__arc_replace(struct __arc_s *state, struct __arc_l *list)
{
  if (state->mru.size == state->c || (state->mru.size >= 1 && ((list == &state->mfug && state->mru.size == state->p) || (state->mru.size > state->p)))) {
    struct __arc_e *e = __arc_list_remove(&state->mru);
    __arc_list_move(e, &state->mrug);
    state->ops->evict(e);

    return &state->mrug;
  } else {
    struct __arc_e *e = __arc_list_remove(&state->mfu);
    __arc_list_move(e, &state->mfug);
    state->ops->evict(e);

    return &state->mfug;
  }
}

static struct __arc_e *__search_in(struct __arc_s *state, struct __list *list, const void *key)
{
  struct __list *pos;
  __list_each(pos, list) {
    struct __arc_e *e = __list_entry(pos, struct __arc_e, head);
    if (state->ops->cmp(e, key) == 0)
      return e;
  }

  return NULL;
}

static struct __arc_e *__search(struct __arc_s *state, const void *key)
{
  struct __arc_e *e;

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

struct __arc_e *__arc_lookup(struct __arc_s *state, const void *key)
{
  struct __arc_e *entry = __search(state, key);

  if (entry == NULL) {
    //    printf("lookup: cache miss\n");
    if (state->mru.size + state->mfu.size == state->c) {
      struct __arc_l *l = __arc_replace(state, NULL);
      if (state->mrug.size + state->mru.size + state->mfu.size + state->mfug.size == 2 * state->c) {
	struct __arc_e *e = __list_entry(l->head.prev, struct __arc_e, head);
	__arc_list_move(e, NULL);
      }
    }

    entry = state->ops->alloc(key);
    __arc_list_move(entry, &state->mru);
  } else if (entry->list == &state->mfug) {
    state->p = MAX(0, state->p - MAX(state->mrug.size / state->mfug.size, 1));
    //printf("mfu ghost, p=%d\n", state->p);
    __arc_replace(state, entry->list);
    __arc_list_move(entry, &state->mfu);

    state->ops->fetch(entry);
  } else if (entry->list == &state->mrug) {
    state->p = MAX(0, state->p + MAX(state->mfug.size / state->mrug.size, 1));
    //printf("mru ghost, p=%d\n", state->p);
    __arc_replace(state, entry->list);
    __arc_list_move(entry, &state->mfu);

    state->ops->fetch(entry);
  } else {
    //printf("mfu/mru\n");
    __arc_list_move(entry, &state->mfu);
  }

  return entry;
}

/*
void __arc_access(struct __arc_s *state, struct __arc_e *entry)
{
  __arc_list_remove(entry);

  if (entry->list == &state->mru) {
    __arc_list_insert(entry, &state->mfu);
  } else if (entry->list == &state->mrug) {
    __arc_list_insert(entry, &state->mfu);
  } else if (entry->list == &state->mfu) {
    __arc_list_insert(entry, &state->mfu);
  } else if (entry->list == &state->mfug) {
    __arc_list_insert(entry, &state->mfu);
  } else {
    __arc_list_insert(entry, &state->mru);
  }
}

void __arc_get(struct __arc_e *entry)
{
  __arc_list_remove(entry);
  //  __arc_list_insert(entry, &state->locked);
}

void __arc_put(struct __arc_e *entry)
{
  __arc_list_remove(entry);
  //  __arc_list_insert(entry, &state->mfu);
}
*/
