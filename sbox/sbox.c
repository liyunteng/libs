/*
 * sbox.c - sbox
 *
 * Date   : 2020/04/29
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "sbox.h"

int sbox_cmp(sbox_ptr a, sbox_ptr b)
{
    uint32_t i;
    int j;

    if (a == b) {
        return 0;
    } else if (a == NULL) {
        return -1;
    } else if (b == NULL) {
        return 1;
    } else {
        switch(a->type) {
        case so_int:
            return a->d.i == b->d.i ? 0 : (a->d.i > b->d.i ? 1 : -1);
        case so_float:
            return fabs(a->d.f - b->d.f) < FACCURACY ? 0 : (a->d.f > b->d.f ? 1 : -1);
        case so_str:
            return strcmp(a->d.c, b->d.c);
        case so_bytes:
            i = min(a->len, b->len);
            j = memcpy((uint8_t*)a->d.b, (uint8_t *)b->d.b, i);
            if (j != 0) {
                return j;
            } else {
                return a->len == b->len ? 0 : (a->len > b->len ? 1 : -1);
            }
        default:
            return a > b ? 1 : -1;
        }
    }
}

sbox_ptr sbox_new_int(int64_t i)
{
    sbox_ptr box = (sbox_ptr)malloc(sizeof(sbox_t));
    if (box) {
        box->type = so_int;
        box->len = sizeof(int64_t);
        atomic_inc(&box->count);
        box->d.i = i;
    }
    return box;
}

sbox_ptr sobx_new_float(double d)
{
    sbox_ptr box = (sbox_ptr)malloc(sizeof(sbox_t));
    if (box) {
        box->type = so_float;
        box->len = sizeof(double);
        atomic_inc(&box->count);
        box->d.f = d;
    }
    return box;
}

sbox_ptr sbox_new_str(const char *str)
{
    uint32_t size, len;
    sbox_ptr box = NULL;
    if (str == NULL) {
        return NULL;
    }
    len = strlen(str) + 1;
    size = sbox_size(len);
    box = (sbox_ptr)malloc(size);
    if (box) {
        memset(box, 0, len);
        box->type = so_str;
        box->len = len;
        atomic_inc(&box->count);
        strcpy(box->d.c, str);
    }
    return box;
}

sbox_ptr sbox_new_bytes(void *data, uint32_t len)
{
    uint32_t size = sbox_size(len);
    sbox_ptr box = NULL;
    if (data == NULL) {
        return NULL;
    }
    box = (sbox_ptr)malloc(size);
    if (box) {
        memset(box, 0, size);
        box->type = so_bytes;
        box->len = len;
        atomic_inc(&box->count);
        memcpy(box->d.b, data, len);
    }
    return box;
}

sbox_ptr sbox_new_dict()
{
    sbox_ptr box = (sbox_ptr)malloc(sizeof(sbox_t));
    if (box) {
        memset(box, 0, sizeof(sbox_t));
        box->type = so_dict;
        box->len = sizeof(sbox_t);
        atomic_inc(&box->count);
    }
    return box;
}

sbox_ptr sbox_hold(sbox_ptr box)
{
    if (box) {
        /* FIXME: should inc only if not zero */
        atomic_inc(&box->count);
    }
}

static sbox_kv_pair *__dict_get(sbox_ptr dict, sbox_ptr key)
{
    sbox_kv_pair *node = NULL;
    struct rb_node *p = dict->d.root.rb_node;
    int i;
    while(p) {
        node = rb_entry(p, sbox_kv_pair, node);
        i = sbox_cmp(key, node->key);
        if (i < 0) {
            p = p->rb_left;
        } else if (i > 0) {
            p = p->rb_right;
        } else {
            return node;
        }
    }
    return NULL;
}

static void __dict_insert(sbox_ptr box, sbox_kv_pair *new)
{
    struct rb_node **p = &(box->d.root.rb_node);
    struct rb_node *parent = NULL;
    sbox_kv_pair *node = NULL;
    int i;

    while (*p) {
        parent = *p;
        node = rb_entry(parent, sbox_kv_pair, node);
        i = sbox_cmp(new->key, node->key);
        if (i < 0) {
            p = &(*p)->rb_left;
        } else if (i > 0) {
            p = &(*p)->rb_right;
        } else {
            /* should not happen */
            return;
        }
    }

    rb_link_node(&new->node, parent, p);
    rb_insert_color(&new->node, &(box->d.root));
}

void sbox_dict_insert(sbox_ptr dict, sbox_ptr key, sbox_ptr value)
{
    sbox_kv_pair *node = NULL;
    if (!dict || !key || !value) {
        return;
    }

    node = __dict_get(dict, key);
    if (node) {
        /* update, overwrite old key and value */
        if (node->value != value) {
            sbox_release(node->key);
            sbox_release(node->value);
            node->key = key;
            node->value = value;
        }
    } else {
        /* insert */
        node = (sbox_kv_pair *)malloc(sizeof(sbox_kv_pair));
        if (node) {
            memset(node, 0, sizeof(sbox_kv_pair));
            node->key = key;
            node->value = value;
            __dict_insert(dict, node);
        }
    }
}

sbox_ptr sbox_dict_get(sbox_ptr dict, sbox_ptr key)
{
    sbox_kv_pair *node = NULL;
    sbox_ptr val = NULL;
    if (dict && dict->type == so_dict && key) {
        node = __dict_get(dict, key);
        if (node) {
            val = node->value;
        }
    }
    sbox_release(key);
    return val;
}

void sbox_dict_del(sbox_ptr dict, sbox_ptr key)
{
    sbox_kv_pair *node = NULL;
    if (dict && dict->type == so_dict && key) {
        node = __dict_get(dict, key);
        if (node) {
            rb_erase(&node->node, &(dict->d.root));
            sbox_release(node->key);
            sbox_release(node->value);
            free(node);
        }
    }
    sbox_release(key);
    return;
}

sbox_kv_pair *sbox_dict_first(sbox_ptr dict)
{
    sbox_kv_pair *pair = NULL;
    if (dict && dict->type == so_dict) {
        struct rb_node *first = rb_first(&(dict->d.root));
        if (first) {
            pair = rb_entry(first, sbox_kv_pair, node);
        }
    }
    return pair;
}

sbox_kv_pair *sbox_dict_last(sbox_ptr dict)
{
    sbox_kv_pair *pair = NULL;
    if (dict && dict->type == so_dict) {
        struct rb_node *last = rb_last(&(dict->d.root));
        if (last) {
            pair = rb_entry(last, sbox_kv_pair, node);
        }
    }
    return pair;
}

sbox_kv_pair *sbox_dict_next(sbox_kv_pair *pair)
{
    sbox_kv_pair *next_pair = NULL;
    if (pair) {
        struct rb_node *next = rb_next(&pair->node);
        if (next) {
            next_pair = rb_entry(next, sbox_kv_pair, node);
        }
    }
    return next_pair;
}

sbox_kv_pair *sbox_dict_prev(sbox_kv_pair *pair)
{
    sbox_kv_pair *prev_pair = NULL;
    if (pair) {
        struct rb_node *prev = rb_prev(&pair->node);
        if (prev) {
            prev_pair = rb_entry(prev, sbox_kv_pair, node);
        }
    }
    return prev_pair;
}

sbox_ptr sbox_new_list(void)
{
    sbox_ptr box = (sbox_ptr) malloc(sizeof(sbox_t));
    if (box) {
        memset(box, 0, sizeof(sbox_t));
        box->type = so_list;
        box->len = sizeof(sbox_t);
        atomic_inc(&box->count);
    }
    return box;
}

void sbox_list_append(sbox_ptr list, sbox_ptr data)
{
    sbox_list_node *node = NULL;

    if (!list || !data) {
        return;
    }

    if (list->type != so_list) {
        return;
    }

    node = (sbox_list_node *)malloc(sizeof(sbox_list_node));
    if (node) {
        node->data = data;
        node->next = NULL;
        if (!list->d.h) {
            /* header is null */
            list->d.h = node;
            return;
        } else {
            sbox_list_node *p = list->d.h;
            while (p->next) {
                p = p->next;
            }
            p->next = node;
            return;
        }
    }
}

sbox_ptr sbox_list_get(sbox_ptr list, uint32_t idx)
{
    uint32_t i = 0;
    sbox_list_node *p = list->d.h;
    if (!list || list->type != so_list || !list->d.h) {
        return NULL;
    }

    while (p && i <= idx) {
        if (i == idx) {
            return (sbox_ptr)p->data;
        } else {
            i++;
            p = p->next;
        }
    }
    return NULL;
}

void sbox_list_del(sbox_ptr list, sbox_ptr target)
{
    sbox_list_node *p = NULL;
    sbox_list_node *q = NULL;
    if (!list || list->type != so_list || !target) {
        return;
    }
    p = list->d.h;

    if (p->data == target) {
        q = p;
        list->d.h = q->next;
    } else {
        while (p->next) {
            if (p->next->data == target) {
                q = p->next;
                p->next = q->next;
            }
        }
    }

    if (q) {
        sbox_release(q->data);
        free(q);
    }
}

sbox_list_node *sbox_list_first(sbox_ptr list)
{
    if (!list || list->type != so_list) {
        return NULL;
    }

    return list->d.h;
}

sbox_list_node *sbox_list_next(sbox_list_node *cur)
{
    if (!cur) {
        return NULL;
    }
    return cur->next;
}

static int byte2str(void *data, size_t size, char *buf)
{
    static char *tab = "0123456789ABCDEF";
    uint8_t *d = data;
    int i = 0;
    if (d) {
        for (;i < size; i++) {
            buf[i * 2] = tab[(d[i] >> 4) & 0xF];
            buf[i * 2 + 1] = tab[d[i] & 0xF];
        }
    }
    return i * 2;
}

int sbox_tostr(sbox_ptr box, char *buf)
{
    int offset = 0;
    sbox_list_node *node = NULL;
    sbox_kv_pair *pair = NULL;

    if (box) {
        switch(box->type) {
        case so_int:
            return sprintf(buf, "%ld", box->d.i);

        case so_float:
            return sprintf(buf, "%lf", box->d.f);

        case so_str:
            return sprintf(buf, "\"%s\"", box->d.c);

        case so_bytes:
            buf[offset++] = '"';
            offset += byte2str(box->d.b, box->len, buf+offset);
            buf[offset++] = '"';
            return offset;

        case so_list:
            offset = 0;
            buf[offset++] = '[';
            for (node = sbox_list_first(box); node; node = sbox_list_next(node)) {
                if (offset > 1) {
                    buf[offset++] = ',';
                }
                offset += sbox_tostr(node->data, buf+offset);
            }
            buf[offset++] = ']';
            return offset;

        case so_dict:
            offset = 0;
            buf[offset++] = '{';
            for (pair = sbox_dict_first(box); pair; pair = sbox_dict_next(pair)) {
                if (offset > 1) {
                    buf[offset++] = ',';
                }

                offset += sbox_tostr(pair->key, buf+offset);
                buf[offset++] = ':';
                offset += sbox_tostr(pair->value, buf+offset);
            }
            buf[offset++] = '}';
            return offset;

        default:
            strcpy(buf, "\"unknow\"");
            return 8;
        }
    } else {
        strcpy(buf, "null");
        return 4;
    }
}

static void __dict_free_helper(struct rb_node *rb_node)
{
    sbox_kv_pair *node = NULL;
    if (rb_node) {
        if (rb_node->rb_left) {
            __dict_free_helper(rb_node->rb_left);
        }
        if (rb_node->rb_right) {
            __dict_free_helper(rb_node->rb_right);
        }
        node = rb_entry(rb_node, sbox_kv_pair, node);
        sbox_release(node->key);
        sbox_release(node->value);
        free(node);
    }
    return;
}

static void __list_free_helper(sbox_ptr list)
{
    sbox_list_node *p = list->d.h;
    sbox_list_node *q = NULL;
    while (p) {
        q = p;
        p = q->next;
        sbox_release(q->data);
        free(q);
    }
}

void sbox_release(sbox_ptr box)
{
    if (box) {
        if (atomic_dec_and_test(&box->count)) {
            switch (box->type) {
            case so_list:
                __list_free_helper(box);
                break;
            case so_dict:
                __dict_free_helper(box->d.root.rb_node);
                break;
            default:
                break;
            }
            free(box);
        }
    }
    return;
}
