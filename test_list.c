/*
 * test_list.c -- test list.h
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/04/16 19:23:02
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

#include <stdio.h>
#include <stdlib.h>
#include "list.h"

typedef int datatype;
struct test_list {
	datatype val;
	struct list_head list;
};

int main(void)
{
	/* init head */
	/*
	 * struct test_list tl_head = {LIST_HEAD_INIT(tl_head.list), 0};
	 * struct list_head head = LIST_HEAD_INIT(head);
	 * printf("tl_head: %p, head: %p\n", &tl_head, &head);
	 */

	/*
	 * struct test_list tl_head;
	 * INIT_LIST_HEAD(&tl_head.list);
	 * printf("tl_head :%p, head: %p\n", &tl_head, &tl_head.list);
	 */

	LIST_HEAD(head);
	int i;
	for (i = 0; i < 20; i++) {
		struct test_list *t = (struct test_list *)malloc(sizeof(struct test_list));
		t->val = i;
		list_add_tail(&t->list, &head);
	}

	struct test_list t;
	t.val = 128;

	struct list_head *p;
	struct list_head *tmp;
	list_for_each_safe(p, tmp, &head) {
		if (list_entry(p, struct test_list, list)->val % 2) {
			list_del(p);
			free(list_entry(p, struct test_list, list));
		}
		if (list_entry(p, struct test_list, list)->val == 4) {
			list_replace_init(p, &t.list);
			free(list_entry(p, struct test_list, list));
		}

	}

	/* list_move(&t.list, &head); */

	/* list_move(&t.list, &head); */

	LIST_HEAD(head1);
	list_cut_position(&head1, &head, &t.list);
	struct test_list *pt;
	printf("before splice head:\n");
	list_for_each_entry(pt, &head, list) {
		printf("val: %d\n", pt->val);
	}
	printf("befor splice head1:\n");
	list_for_each_entry(pt, &head1, list) {
		printf("val: %d\n", pt->val);
	}
	list_splice_tail(&head1, &head);
	printf("after splice head:\n");
	list_for_each_entry(pt, &head, list) {
		printf("val: %d\n", pt->val);
	}
	list_rotate_left(&head);
	printf("after rotate\n");
	list_for_each_entry(pt, &head, list) {
		printf("val: %d\n", pt->val);
	}

	list_for_each_safe(p, tmp, &head) {
		if (list_entry(p, struct test_list, list)->val != 128) {
			list_del(p);
			free(list_entry(p, struct test_list, list));
		}
	}
	if (list_is_singular(&head)) {
		printf("is singular val :%d\n", list_first_entry(&head, struct test_list, list)->val);
	}

	list_del(&t.list);
	printf("after del\n");
	if (list_empty(&head)) {
		printf("is empty.\n");
	}

	list_for_each(p, &head) {
		printf("val: %d\n\n", list_entry(p, struct test_list, list)->val);
	}

	printf("head: %p\n", &head);
	return 0;
}
