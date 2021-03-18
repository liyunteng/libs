/*
 * test.c - test
 *
 * Date   : 2021/03/18
 */

#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


typedef int datatype;
typedef struct {
    datatype val;
    struct list_head list;
} data_t;

static struct list_head head = LIST_HEAD_INIT(head);
static void
print_list(struct list_head *h)
{
    data_t *p = NULL;
    list_for_each_entry(p, h, list) { printf("%d ", p->val); }

    /* struct list_head *p = NULL;
     * list_for_each(p, h) {
     *     printf("%d ", list_entry(p, data_t, list)->val);
     * } */
    printf("\n");
}

int
list_test()
{
    int count           = 100;
    struct list_head *x = &head;
    for (int i = 0; i < count; i++) {
        data_t *d = (data_t *)malloc(sizeof(data_t));
        d->val    = i;
        list_add(&d->list, x);
        x = &d->list;
        /* list_add(&d->list, &head); */
        /* list_add_tail(&d->list, &head); */
    }

    print_list(&head);

    data_t data[100];
    for (int i = 0; i < 100; i++) {
        data[i].val = 0;
        INIT_LIST_HEAD(&data[i].list);
    }

    data_t *p = NULL;
    data_t *n = NULL;
    int c     = 0;
    list_for_each_entry_safe(p, n, &head, list)
    {
        if (p->val % 2 == 0) {
            /* list_del(&p->list); */
            list_replace(&p->list, &data[c++].list);
            /* n = container_of(data.list.next, data_t, list); */
        }
    }
    print_list(&head);

    p = list_first_entry(&head, data_t, list);
    p = list_entry(p->list.next, data_t, list);
    printf("%d\n", p->val);
    return 0;
}

int
main(void)
{
    list_test();
    return 0;
}
