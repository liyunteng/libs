/*
 * list.h -- my list from kernel
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/04/15 18:38:57
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef LIST_H
#define LIST_H

#define POISON_POINTER_DELTA	0
#define LIST_POISON1		((void *)0x100 + POISON_POINTER_DELTA)
#define LIST_POISON2		((void *)0x200 + POISON_POINTER_DELTA)

#define barrier()	 __asm__ __volatile__("":::"memory")
#define __force		 __attribute__((force))
#define __always_inline  __attribute__((always_inline))

struct list_head {
	struct list_head *prev, *next;
};

typedef unsigned char  __u8;
typedef unsigned short __u16;
typedef unsigned int   __u32;
typedef unsigned long  __u64;

static __always_inline void __write_once_size(volatile void *p, void *res, int size)
{
	switch (size) {
	case 1: *(volatile __u8 *)p = *(__u8 *)res; break;
	case 2: *(volatile __u16 *)p = *(__u16 *)res; break;
	case 4: *(volatile __u32 *)p = *(__u32 *)res; break;
	case 8: *(volatile __u64 *)p = *(__u64 *)res; break;
	default:
		barrier();
		__builtin_memcpy((void *)p, (const void *)res, size);
		barrier();
	}
}

#define WRITE_ONCE(x, val)                                              \
	({                                                              \
		union { typeof(x) __val; char __c[1]; } __u =           \
			{ .__val = (__force typeof(x)) (val) }; \
		__write_once_size(&(x), __u.__c, sizeof(x));            \
		__u.__val;                                              \
	})

#define __READ_ONCE_SIZE			\
	({					\
		switch (size) {			\
		case 1: *(__u8 *)res = *(volatile __u8 *)p; break;	\
		case 2: *(__u16 *)res = *(volatile __u16 *)p; break;	\
		case 4: *(__u32 *)res = *(volatile __u32 *)p; break;	\
		case 8: *(__u64 *)res = *(volatile __u64 *)p; break;	\
		default:						\
			barrier();					\
			__builtin_memcpy((void *)res, (const void *)p, size); \
			barrier();					\
		}							\
	})

static __always_inline
void __read_once_size(const volatile void *p, void *res, int size)
{
	__READ_ONCE_SIZE;
}

static __always_inline
void __read_once_size_nocheck(const volatile void *p, void *res, int size)
{
	__READ_ONCE_SIZE;
}

#define __READ_ONCE(x, check)			\
	({					\
		union { typeof(x) __val; char __c[1]; } __u;	\
		if (check)					\
			__read_once_size(&(x), __u.__c, sizeof(x));	\
		else							\
			__read_once_size_nocheck(&(x), __u.__c, sizeof(x)); \
		__u.__val;						\
	})
#define READ_ONCE(x) __READ_ONCE(x, 1)

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name)                                 \
	struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	WRITE_ONCE(list->next, list);
	list->prev = list;
}

/*
 * Insert a new entry between two known cosecutive entries.
 *
 * This is only for internal list manipulation where we know the
 * prev/next entrie already!
 */
static inline void __list_add(struct list_head *new,
			struct list_head *prev,
			struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	WRITE_ONCE(prev->next, new);
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry tobe added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

/* Delete a list entry by making the prev/next entries point to each
 * other.
 *
 * this is only for internal list manipulation where we know the
 * prev/next entries already!
 */
static inline void __list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	WRITE_ONCE(prev->next, next);
}

/**
 * list_del - delete entry from list.
 * @entry: the element to delete from the list.
 *
 * NOTE:
 * list_empty() on entry does not return ture after this, the
 * entry is in an undefined state.
 */
static inline void __list_del_entry(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->next = LIST_POISON2;
}

/**
 * list_replace - replace old entry by new one
 * @old: the element to be replaced
 * @new: the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void list_replace(struct list_head *old,
				struct list_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

static inline void list_replace_init(struct list_head *old,
				struct list_head *new)
{
	list_replace(old, new);
	INIT_LIST_HEAD(old);
}
/**
 * list_del_init - deletes entry from list and reinitialize it
 * @entry: the element to delete from list
 */
static inline void list_del_init(struct list_head *entry)
{
	__list_del_entry(entry);
	INIT_LIST_HEAD(entry);
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void list_move(struct list_head *list,
			struct list_head *head)
{
	__list_del_entry(list);
	list_add(list, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 *
 */
static inline void list_move_tail(struct list_head *list,
				struct list_head *head)
{
	__list_del_entry(list);
	list_add_tail(list, head);
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int list_is_last(const struct list_head *list,
			const struct list_head *head)
{
	return list->next == head;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int list_empty(const struct list_head *head)
{
	return READ_ONCE(head->next) == head;
}

/**
 * list_empty_careful - tests whether a list is empty and not being modified
 * @head: the list to test
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might
 * be in the process of modifying either member (next or prev)
 *
 * NOTE:
 * using list_empty_careful() without synchronization can only be safe
 * if the only activity that can happen to the list entry is
 * list_del_init(). Eg. it cannot be used if another CPU could
 * re-list_add() it.
 */
static inline int list_empty_careful(const struct list_head *head)
{
	struct list_head *next = head->next;
	return (next == head) && (next == head->prev);
}

/**
 * list_rotate_left - rotate the list to the left
 * @head: the head of the list
 */
static inline void list_rotate_left(struct list_head *head)
{
	struct list_head *first;

	if (!list_empty(head)) {
		first = head->next;
		list_move_tail(first, head);
	}
}

/**
 * list_is_singular - tests whether a list has just one entry
 * @head: the list to test.
 */
static inline int list_is_singular(const struct list_head *head)
{
	return !list_empty(head) && (head->next == head->prev);
}

static inline void __list_cut_position(struct list_head *list,
				struct list_head *head,
				struct list_head *entry)
{
	struct list_head *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}

/**
 * list_cut_position - cut a list into two
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself and if so we
 *	 won't cut the list
 *
 * Description:
 * This helper moves the initial part of @head, up to and including
 * @entry, from @head to @list. You should pass on @entry and element
 * you know is on @head. @list should be an empty list or an list you
 * do not care about losing its data.
 *
 */
static inline void list_cut_position(struct list_head *list,
				struct list_head *head,
				struct list_head *entry)
{
	if (list_empty(head))
		return ;
	if (list_is_singular(head) &&
		(head->next != entry && head != entry))
		return ;
	if (entry == head)
		INIT_LIST_HEAD(list);
	else
		__list_cut_position(list, head, entry);
}

static inline void __list_splice(const struct list_head *list,
				struct list_head *prev,
				struct list_head *next)
{
	struct list_head *first = list->next;
	struct list_head *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice(const struct list_head *list,
			struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head, head->next);
}

/**
 * list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice_tail(struct list_head *list,
				struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head->prev, head);
}

/**
 * list_splice_init - join two lists and reinitialize the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Description:
 * The list at @list is reinitialized.
 */
static inline void list_splice_init(struct list_head *list,
				struct list_head *head)
{
	if (!list_empty(list)) {
		__list_splice(list, head, head->next);
		INIT_LIST_HEAD(list);
	}
}

/**
 * list_splice_tail_init - join two lists and reinitialize the emptied list
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Description:
 * Each of the lists is a queue.
 * The list at @list is reinitialize.
 */
static inline void list_splice_tail_init(struct list_head *list,
					strcut list_head *head)
{
	if (!list_empty(list)) {
		__list_splice(list, head->prev, head);
		INIT_LIST_HEAD(list);
	}
}

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({	\
			const typeof( ((type *)0)->member ) *__mptr = (ptr); \
			(type *)( (char *)__mptr - offsetof(type, member) );})
/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
  */
#define list_entry(ptr, type, member)		\
	container_of(ptr, type, member)

/**
 * list_first_entry - get the first element from a list
 * @ptr:	the list head to take element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 *
 * NOTE:
 * that list is expected to be not empty.
 */
#define list_first_entry(ptr, type, member)	\
	list_entry((ptr)->next, type, member)

/**
 * list_last_entryt - get the last element from a list
 * @ptr:	the list head to take the element from .
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 *
 * NOTE:
 * that list is expected to be not empty.
 */
#define list_last_entry(ptr, type, member)		\
	list_entry((ptr)->prev, type, member)
/**
 * list_first_entry_or_null - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 *
 * NOTE:
 * that if the list is empty, it returns NULL
 */
#define list_first_entry_or_null(ptr, type, member)	\
	(!list_empty(ptr) ? list_first_entry(ptr, type, member) : NULL)

/**
 * list_next_entry - get the next element in list
 * @pos:	the type * to cursor
 * @member:	the name of the list_head within the struct.
 */
#define list_next_entry(pos, member)		\
	list_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * list_prev_entry - get the prev element in list
 * @pos:	the type * to cursor
 * @member:	the name of the list_head within the strcut.
 */
#define list_prev_entry(pos, member)		\
	list_entry((pos)->member.prev, typeof(*(pos)), member)

/**
 * list_next_entry - get the next element in list
 * @pos: the type * to cursor
 * @member: the name of the list_head within the struct.
 */
#define list_for_each(pos, head)		\
	for (pos = (head)->next; pos != (head); pos = pos->next)
#endif
