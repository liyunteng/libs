/*
 * test.c - test
 *
 * Date   : 2021/03/18
 */
#include "rbtree.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* #ifdef NDEBUG
 * #undef NDEBUG
 * #endif */
#include <assert.h>

#define N 32

typedef struct rbtree_value {
    rbtree_node_t node;
    int value;
} rbtree_value_t;

static void
rbtree_check_node(rbtree_node_t *node, int *black)
{
    int i;
    rbtree_node_t *parent;

    assert(node);
    if (0 == node->color) {
        // red node parent must be black
        assert(node->parent->color);
        // red node child must be null or two black node
        assert((!node->left && !node->right)
               || (node->left->color && node->right->color));
    }

    if (!node->left || !node->right) {
        // check black node count
        i = node->color ? 1 : 0;
        for (parent = node->parent; parent; parent = parent->parent) {
            if (parent->color)
                i++;
        }
        assert(0 == *black || i == *black);
        *black = i;
    }

    if (node->left)
        rbtree_check_node(node->left, black);

    if (node->right)
        rbtree_check_node(node->right, black);
}

static void
rbtree_validate(rbtree_root_t *root)
{
    int black = 0;
    if (root->node)
        rbtree_check_node(root->node, &black);
}

static char s_tree[256];
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

static void
rbtree_print_node(rbtree_node_t *node)
{
    rbtree_node_t *p;
    rbtree_node_t *g;
    rbtree_value_t *v;
    if (!node)
        return;

    p = node->parent;
    g = p ? p->parent : NULL;

    s_tree[0] = 0;
    while (g) {
        if (p == g->left && g->right) {
            strcat(s_tree, "\t|");
        } else {
            strcat(s_tree, "\t");
        }

        p = g;
        g = p->parent;
    }
    strrev(s_tree);

    v = rbtree_entry(node, rbtree_value_t, node);

    if (!node->parent)
        printf("%d(%c)\n", v->value, v->node.color ? 'B' : 'R');
    else
        printf("%s|----%d(%c)\n", s_tree, v->value, v->node.color ? 'B' : 'R');

    rbtree_print_node(node->left);
    rbtree_print_node(node->right);
}

static void
rbtree_print(rbtree_root_t *root)
{
    rbtree_print_node(root->node);
}

static int
rbtree_find(rbtree_root_t *root, int v, rbtree_node_t **node)
{
    int r;
    rbtree_node_t *link;
    rbtree_value_t *value;

    r     = -1;
    *node = NULL;
    link  = root->node;
    while (link) {
        *node = link;
        value = rbtree_entry(link, rbtree_value_t, node);
        r     = value->value - v;
        if (0 == r)
            break;

        link = r > 0 ? link->left : link->right;
    }

    return r;
}

static void
rbtree_iter_test(void)
{
    int r, i;
    rbtree_root_t root;
    rbtree_node_t **link;
    rbtree_node_t *parent;
    const rbtree_node_t *node;
    rbtree_value_t *value;

    root.node = NULL;
    for (i = 0; i < 100000; i++) {
        r = rbtree_find(&root, i, &parent);
        assert(0 != r);
        link = parent ? (r > 0 ? &parent->left : &parent->right) : NULL;

        value        = (rbtree_value_t *)malloc(sizeof(*value));
        value->value = i;
        rbtree_insert(&root, parent, link, &value->node);
    }

    node = rbtree_first(&root);
    for (i = 0; i < 100000; i++) {
        value = rbtree_entry(node, rbtree_value_t, node);
        assert(i == value->value);
        node = rbtree_next(node);
    }

    node = rbtree_last(&root);
    for (i = 0; i < 100000; i++) {
        value = rbtree_entry(node, rbtree_value_t, node);
        assert(100000 - 1 - i == value->value);
        node = rbtree_prev(node);
    }
}

void
rbtree_test(void)
{
    int i, v[N];
    rbtree_root_t root;
    rbtree_node_t *parent;
    rbtree_node_t **link;
    rbtree_value_t *value;
    int ret;
    root.node = NULL;

    int seed = (int)time(NULL);  // 1506061127 (duplicate)
    srand(seed);
    for (i = 0; i < N; i++) {
        /* v[i] = rand(); */
        v[i] = i;

        parent = NULL;
        link   = &root.node;
        while (*link) {
            parent = *link;
            value  = rbtree_entry(parent, rbtree_value_t, node);
            // duplicate node insert at right children tree
            link = value->value > v[i] ? &parent->left : &parent->right;
        }

        value        = (rbtree_value_t *)malloc(sizeof(*value));
        value->value = v[i];
        rbtree_insert(&root, parent, link, &(value->node));
        rbtree_validate(&root);
    }
    rbtree_print(&root);

    for (i = 0; i < N; i++) {
        int m = rand() % N;
        int n = rand() % N;
        int l = v[m];
        v[m]  = v[n];
        v[n]  = l;
    }

    for (i = 0; i < N / 2; i++) {
        ret = rbtree_find(&root, v[i], &parent);
        assert(0 == ret);
        value = rbtree_entry(parent, rbtree_value_t, node);
        assert(value->value == v[i]);
        rbtree_delete(&root, parent);
        rbtree_validate(&root);
    }
    rbtree_print(&root);

    rbtree_iter_test();
    printf("rb-tree test ok\n");
}

typedef rbtree_root_t dict_root_t;

typedef struct dict {
    struct rbtree_node node;
    uint32_t key;
    char value[128];
} dict_t;

#define DICT_ROOT_INIT                                                         \
    (dict_root_t) { NULL, }
#define dict_root_init(root) ((root)->node = NULL)

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
    int result               = 0;
    rbtree_node_t *node = root->node;

    while (node) {
        dict_t *data = rbtree_entry(node, dict_t, node);
        result       = dict_key_cmp(key, data->key);
        if (result == 0) {
            return data;
        }
        node = result > 0 ? node->right : node->left;
    }

    return NULL;
}

int
dict_insert(dict_root_t *root, dict_t *data)
{
    int result            = 0;
    rbtree_node_t **link  = &(root->node);
    rbtree_node_t *parent = NULL;

    while (*link) {
        parent = *link;
        dict_t *this = rbtree_entry(parent, dict_t, node);
        result = dict_key_cmp(data->key, this->key);
         // duplicated key link to left
        link = result > 0 ? &parent->right : &parent->left;
    }

    rbtree_insert(root, parent, link, &(data->node));
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
        rbtree_delete(root, &data->node);
        dict_destroy(data);
    }
}

typedef void (*dict_iter_callback)(int idx, dict_t *data);
void
dict_iter(dict_root_t *root, dict_iter_callback callback)
{
    const rbtree_node_t *p = rbtree_first(root);
    uint32_t idx      = 0;
    while (p) {
        dict_t *data = rbtree_entry(p, dict_t, node);
        if (data && callback) {
            callback(idx, data);
        }
        idx++;
        p = rbtree_next(p);
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
    dict_iter(&root, dump);
    rbtree_print(&root);

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
    /* rbtree_test(); */
    dict_test();
    return 0;
}
