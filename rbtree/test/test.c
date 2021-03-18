/*
 * test.c - test
 *
 * Date   : 2020/04/26
 */
#include "rbtree.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct rb_root dict_root_t;

typedef struct dict {
    struct rb_node node;
    uint32_t key;
    char value[128];
} dict_t;

#define DICT_ROOT_INIT                                                         \
    (dict_root_t) { NULL, }
#define dict_root_init(root) ((root)->rb_node = NULL)

static int
dict_key_cmp(const uint32_t first, const uint32_t second)
{
    if (first < second) {
        return -1;
    } else if (first > second) {
        return 1;
    }
    return 0;
}

static char *
strrev(char *str)
{
    char *p1, *p2;

    if (!str || !*str)
        return str;

    for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2) {
        *p1 ^= *p2;
        *p2 ^= *p1;
        *p1 ^= *p2;
    }

    return str;
}

static char s_tree[256];
static void
rbtree_print_node(struct rb_node *node)
{
    struct rb_node *p;
    struct rb_node *g;
    dict_t *v;
    if (!node)
        return;

    p = rb_parent(node);
    g = p ? rb_parent(p) : NULL;

    s_tree[0] = 0;
    while (g) {
        if (p == g->rb_left && g->rb_right) {
            strcat(s_tree, "\t|");
        } else {
            strcat(s_tree, "\t");
        }

        p = g;
        g = rb_parent(p);
    }
    strrev(s_tree);

    v = rb_entry(node, dict_t, node);

    if (!rb_parent(node))
        printf("%d(%c)\n", v->key, rb_is_black(node) ? 'B' : 'R');
    else
        printf("%s|----%d(%c)\n", s_tree, v->key, rb_is_black(node) ? 'B' : 'R');

    rbtree_print_node(node->rb_left);
    rbtree_print_node(node->rb_right);
}


static void
rbtree_print(struct rb_root *root)
{
    rbtree_print_node(root->rb_node);
}

dict_t *
dict_create(uint32_t key, const char *value)
{
    dict_t *dict = NULL;
    dict         = (dict_t *)calloc(1, sizeof(dict_t));
    if (dict) {
        dict->key = key;
        strncpy(dict->value, value, sizeof(dict->value));
    }
    return dict;
}

void
dict_destroy(dict_t *dict)
{
    if (dict) {
        free(dict);
        dict = NULL;
    }
}

dict_t *
dict_search(dict_root_t *root, const uint32_t key)
{
    int result           = 0;
    struct rb_node *node = root->rb_node;

    while (node) {
        dict_t *data = rb_entry(node, dict_t, node);
        result       = dict_key_cmp(key, data->key);
        if (result == 0) {
            return data;
        }

        node = result > 0 ? node->rb_right : node->rb_left;
    }

    return NULL;
}

int
dict_insert(dict_root_t *root, dict_t *data)
{
    int result                = 0;
    struct rb_node **link = &(root->rb_node);
    struct rb_node *parent = NULL;

    while (*link) {
        parent = *link;
        dict_t *this = rb_entry(parent, dict_t, node);

        result      = dict_key_cmp(data->key, this->key);
        // duplicated key link to left
        link = result > 0 ? &parent->rb_right : &parent->rb_left;
    }

    rb_link_node(&data->node, parent, link);
    rb_insert_color(&data->node, root);
    return 0;
}

void
dict_erase(dict_root_t *root, const uint32_t key)
{
    while (1) {
        dict_t *data = dict_search(root, key);
        if (!data) {
            break;
        }
        rb_erase(&data->node, root);
        dict_destroy(data);
    }
}

typedef void (*dict_iter_callback)(int idx, dict_t *data);
void
dict_iter(dict_root_t *root, dict_iter_callback callback)
{
    struct rb_node *p = rb_first(root);
    uint32_t idx      = 0;
    while (p) {
        dict_t *data = rb_entry(p, dict_t, node);
        if (data && callback) {
            callback(idx, data);
        }
        idx++;
        p = rb_next(p);
    }
}

void
dump(int idx, dict_t *data)
{
    printf("%u key: %u  value: %s\n", idx, data->key, data->value);
}

int
dict_test(void)
{
    dict_root_t root = DICT_ROOT_INIT;
    dict_t *data     = NULL;
    char buf[128]    = {0};
    int i;

    for (i = 0; i < 100; i++) {
        snprintf(buf, sizeof(buf) - 1, "this is %d", i);
        data = dict_create(i, buf);
        assert(data);
        dict_insert(&root, data);
    }

    for (i = 0; i < 100; i += 3) {
        snprintf(buf, sizeof(buf) - 1, "this is %d", i + 100);
        data = dict_create(i, buf);
        assert(data);
        dict_insert(&root, data);
    }

    dict_erase(&root, 75);
    rbtree_print(&root);
    dict_iter(&root, dump);

    data = dict_search(&root, 26);
    if (data) {
        printf("key: %u value: %s\n", data->key, data->value);
    }

    for (i = 0; i < 100; i++) {
        dict_erase(&root, i);
    }
    dict_iter(&root, dump);
    return 0;
}

int
main(void)
{
    dict_test();
    return 0;
}
