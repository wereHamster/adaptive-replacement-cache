
#ifndef __LIST_H__
#define __LIST_H__

#include <memory.h>
#include <stddef.h>

struct __list {
  struct __list *prev, *next;
};

#define __list_entry(ptr,type,field) ((type*) (((char*)ptr) - offsetof(type,field)))

#define __list_each(pos, head)			\
  for (pos = (head)->next; pos != (head); pos = pos->next)

#define __list_each_prev(pos, head)	  \
  for (pos = (head)->prev; pos != (head); pos = pos->prev)

typedef int  ( *__list_cmp_t ) ( const void * a, const void * b );
typedef void ( *__list_free_t )( void * );

static void
__list_init( struct __list * head )
{
  head->next = head->prev = head;
}



static void
__list_insert( struct __list * list,
                  struct __list * prev,
                  struct __list * next)
{
  next->prev = list;
  list->next = next;
  list->prev = prev;
  prev->next = list;
}

static void
__list_splice( struct __list * prev,
                  struct __list * next)
{
  next->prev = prev;
  prev->next = next;
}


static void
__list_remove( struct __list * head )
{
  __list_splice( head->prev, head->next );
  head->next = head->prev = NULL;
}

static void
__list_prepend(struct __list *head, struct __list *list)
{
  __list_insert(head, list, list->next );
}

#endif /* __LIST_H__ */
