/*
 * bst.h - bst
 *
 * Date   : 2021/03/18
 */
#ifndef __BST_NO_PARENT_H__
#define __BST_NO_PARENT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef int datatype;
struct bst_node {
    struct bst_node *left;
    struct bst_node *right;
    datatype data;
};

extern struct bst_node *bst_insert(struct bst_node **root, datatype data);
extern struct bst_node *bst_create(struct bst_node **root, datatype a[],
                                   int len);
extern void bst_pre_iterate(struct bst_node *root);
extern void bst_in_iterate(struct bst_node *root);
extern void bst_post_iterate(struct bst_node *root);
extern struct bst_node *bst_min(struct bst_node *root);
extern struct bst_node *bst_max(struct bst_node *root);
extern struct bst_node *bst_search(struct bst_node *root, datatype key);
extern int bst_delete(struct bst_node **root, datatype key);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
