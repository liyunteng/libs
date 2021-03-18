/*
 * bst.h - bst
 *
 * Date   : 2021/03/18
 */
#ifndef __BST_H__
#define __BST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

typedef int datatype;
typedef struct _Node Node;
struct _Node {
    Node *parent;
    Node *lchild;
    Node *rchild;
    datatype data;
};

extern Node *bst_insert(Node **root, datatype data);
extern Node *bst_create(Node **root, datatype arr[], size_t len);
extern void bst_pre_iterate(Node *root);
extern void bst_in_iterate(Node *root);
extern void bst_post_iterate(Node *root);
extern Node *bst_min(Node *root);
extern Node *bst_max(Node *root);
extern Node *bst_search(Node *root, datatype data);
/**
 * bst_delete - delete the Node of data from root
 * @root:     the root of bst
 * @data:     the data to delete
 *
 * RETURN:
 * 0 delet success.
 * -1 data not found.
 * -2 internel error.
 */
extern int bst_delete(Node *root, datatype data);

extern void bst_destroy(Node **root);

#ifdef __cplusplus
}
#endif
#endif
