/*
 * sbox.h - sbox
 *
 * Date   : 2020/04/28
 */
#ifndef __SBOX_H__
#define __SBOX_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "rbtree.h"
#include <stdint.h>
#include <string.h>

#define FACCURACY 0.000001

typedef int32_t atomic_t;
typedef enum __sbox_type {
    so_int,
    so_float,
    so_str,
    so_bytes,
    so_list,
    so_dict,
} sbox_type;

typedef struct rb_root sbox_dict_root;
typedef struct __sbox_list_node {
    void*    data;
    struct __sbox_list_node *next;
} sbox_list_node;

typedef struct __sbox {
    uint32_t type;
    uint32_t len;       //data length
    atomic_t count;
    union {
        int64_t i;      //int
        double  f;      //float
        char    c[0];    //char
        uint8_t b[0];    //bytes
        sbox_dict_root root; //dict
        sbox_list_node* h; //list header
    } d;                //must be the last member
} sbox_t, *sbox_ptr;

#define SBOX_DATA_LEN_DEFAULT ((uint32_t)(sizeof(sbox_t) - offsetof(sbox_t, d)))
#define sbox_size(len) (MAX(len, SBOX_DATA_LEN_DEFAULT) + (uint32_t)offsetof(sbox_t, d))

typedef struct __sbox_dict_node {
    sbox_ptr        key;
    sbox_ptr        value;
    struct rb_node  node;
} sbox_kv_pair;

typedef struct __sbox_iter {
    uint32_t type;
    union {
        sbox_kv_pair *pair;
        sbox_list_node *node;
    }d;
} sbox_iter_t;

static inline int64_t sbox_get_int(sbox_ptr box)
{
    if(box && box->type == so_int) {
        return box->d.i;
    } else {
        return (int64_t)-1;
    }
}

static inline double sbox_get_float(sbox_ptr box)
{
    if(box && box->type == so_float) {
        return box->d.f;
    } else {
        return (double)0.0;
    }
}

static inline char* sbox_get_str(sbox_ptr box)
{
    if(box && box->type == so_str) {
        return box->d.c;
    } else {
        return "Argument was not a string!";
    }
}

static inline ssize_t sbox_get_bytes(sbox_ptr box, void* buff, size_t size)
{
    if(box && box->type == so_bytes) {
        if(size < box->len) {
            return (ssize_t)-1;
        } else {
            memcpy(buff, box->d.b, box->len);
            return (ssize_t)box->len;
        }
    } else {
        return (ssize_t)-1;
    }
}

static inline void* sbox_get_ptr(sbox_ptr box)
{
    if(box) {
        return box->d.b;
    }
    return NULL;
}

extern int sbox_cmp(sbox_ptr a, sbox_ptr b);
extern int sbox_tostr(sbox_ptr box, char* buff);

extern sbox_ptr sbox_new_int(int64_t i);
extern sbox_ptr sbox_new_float(double d);
extern sbox_ptr sbox_new_str(const char* str);
extern sbox_ptr sbox_new_bytes(void* data, uint32_t len);
extern sbox_ptr sbox_new_dict(void);
extern sbox_ptr sbox_new_list(void);

extern void sbox_dict_insert(sbox_ptr dict, sbox_ptr key, sbox_ptr value);
extern sbox_ptr sbox_dict_get(sbox_ptr dict, sbox_ptr key);
extern void sbox_dict_del(sbox_ptr dict, sbox_ptr key);

extern sbox_kv_pair* sbox_dict_first(sbox_ptr dict);
extern sbox_kv_pair* sbox_dict_last(sbox_ptr dict);
extern sbox_kv_pair* sbox_dict_next(sbox_kv_pair *pair);
extern sbox_kv_pair* sbox_dict_prev(sbox_kv_pair *pair);

extern void sbox_list_append(sbox_ptr list, sbox_ptr data);
extern sbox_ptr sbox_list_get(sbox_ptr list, uint32_t idx);
extern void sbox_list_del(sbox_ptr list, sbox_ptr target);
extern sbox_list_node* sbox_list_first(sbox_ptr list);
extern sbox_list_node* sbox_list_next(sbox_list_node *cur);

extern sbox_ptr sbox_hold(sbox_ptr box);
extern void sbox_release(sbox_ptr box);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
