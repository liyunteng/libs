/*
 * test.c - test
 *
 * Date   : 2020/04/26
 */
#include <stdio.h>
#include <string.h>
#include "rbtree.h"

struct test_node_key {
    int id;
};
struct test_node_value {
    char value[128];
};

struct test_rb_node {
    struct rb_node node;
    struct test_node_key key;
    struct test_node_value value;
};

static struct rb_root test_rb_root;
static int test_node_cmp(const struct test_node_key *first,
                         const struct test_node_key *second)
{
    if (first->id > second->id) {
        return 1;
    } else if (first->id < second->id) {
        return -1;
    } else {
        return 0;
    }
}

struct test_rb_node *test_rb_node_create()
{
    struct test_rb_node *node = NULL;

    node = (struct test_rb_node *)malloc(sizeof(struct test_rb_node));
    if (!node) {
        return NULL;
    }
    memset(node, 0, sizeof(struct test_rb_node));
    return node;
}

void test_rb_node_free(struct test_rb_node *node)
{
    if (!node)
        return;

    free(node);
    node = NULL;
}

struct test_rb_node *test_rbtree_search(const struct test_node_key *key)
{
    int result = 0;
    struct rb_node *node = test_rb_root.rb_node;

    while(node) {
        struct test_rb_node *data = rb_entry(node, struct test_rb_node, node);
        result = test_node_cmp(key, &data->key);
        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return data;
    }

    return NULL;
}

int test_rbtree_insert(struct test_rb_node *data)
{
    int result = 0;
    struct rb_node **new_node = &(test_rb_root.rb_node), *parent_node = NULL;

    while (*new_node) {
        struct test_rb_node *this = rb_entry(*new_node, struct test_rb_node, node);

        result = test_node_cmp(&data->key, &this->key);
        parent_node = *new_node;
        if (result < 0)
            new_node = &((*new_node)->rb_left);
        else if (result > 0)
            new_node = &((*new_node)->rb_right);
        else
            return 0;
    }

    rb_link_node(&data->node, parent_node, new_node);
    rb_insert_color(&data->node, &test_rb_root);
    return 1;
}

void test_rbtree_erase(const struct test_node_key *key)
{
    struct test_rb_node *data = test_rbtree_search(key);
    if (data) {
        rb_erase(&data->node, &test_rb_root);
        test_rb_node_free(data);
    }
}

void test_rbtree_init()
{
    test_rb_root = RB_ROOT;
}

void test_rbtree_dump()
{
    struct rb_node *p = rb_first(&test_rb_root);
    while (p) {
        struct test_rb_node *pn = rb_entry(p, struct test_rb_node, node);
        printf("%d: %s color: 0x%lX %p\n", pn->key.id, pn->value.value, pn->node.rb_parent_color, &pn->node);
        p = rb_next(p);
    }
}

int rbtree_test(void)
{
    int array[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    /* int array[] = {35, 18, 69, 9, 21, 60, 90, 4, 30, 45, 64, 85, 96, 50}; */
    test_rbtree_init();
    for (int i = 0; i < sizeof(array)/sizeof(array[0]); i++) {
        struct test_rb_node *node = test_rb_node_create();
        if (node) {
            node->key.id = array[i];
            sprintf(node->value.value, "%d", array[i]);

            test_rbtree_insert(node);
        }
    }
    test_rbtree_dump();
    printf("======\n");
    for (int i = 0; i < sizeof(array)/sizeof(array[0]); i += 2) {
        struct test_node_key key;
        key.id = array[i];
        struct test_rb_node *node = test_rbtree_search(&key);
        if (node) {
            test_rbtree_erase(&key);
        }
    }
    test_rbtree_dump();
    return 0;
}

int main(void)
{
    rbtree_test();
    return 0;
}
