/*
 * rbtree.h - rbtree
 *
 * Date   : 2021/03/18
 */
#ifndef __RBTREE_H__
#define __RBTREE_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "macro.h"

// #pragma pack(push, sizeof(long))
typedef struct rbtree_node {
    struct rbtree_node *left;
    struct rbtree_node *right;
    struct rbtree_node *parent;
    unsigned char color;
} __attribute__((aligned(sizeof(long)))) rbtree_node_t;
// #pragma pack(pop)

typedef struct rbtree_root {
    struct rbtree_node *node;
} rbtree_root_t;

#define rbtree_entry(ptr, type, member)  container_of(ptr, type, member)


/// re-banlance rb-tree(rbtree_link node before)
/// @param[in] root rbtree root node
/// @param[in] parent parent node
/// @param[in] link parent left or right child node address
/// @param[in] node insert node(new node)
void rbtree_insert(rbtree_root_t* root, rbtree_node_t* parent, rbtree_node_t** link, rbtree_node_t* node);

/// re-banlance rb-tree(rbtree_link node before)
/// @param[in] root rbtree root node
/// @param[in] node rbtree new node
void rbtree_delete(rbtree_root_t* root, rbtree_node_t* node);

const rbtree_node_t* rbtree_first(const rbtree_root_t* root);
const rbtree_node_t* rbtree_last(const rbtree_root_t* root);
const rbtree_node_t* rbtree_prev(const rbtree_node_t* node);
const rbtree_node_t* rbtree_next(const rbtree_node_t* node);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
