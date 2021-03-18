/*
 * bst.c - bst
 *
 * Date   : 2021/03/18
 */
#include "bst.h"

#include <stdio.h>
#include <stdlib.h>

struct bst_node *
bst_insert(struct bst_node **root, datatype data)
{
    if (!*root) {
        *root = (struct bst_node *)malloc(sizeof(struct bst_node));
        if (!*root) {
            fprintf(stderr, "malloc failed.\n");
            return NULL;
        }
        (*root)->left = (*root)->right = NULL;
        (*root)->data                  = data;
        return *root;
    }

    if (data > (*root)->data)
        bst_insert(&(*root)->right, data);
    else
        bst_insert(&(*root)->left, data);

    return NULL;
}

struct bst_node *
bst_create(struct bst_node **root, datatype a[], int len)
{
    for (int i = 0; i < len; i++) {
        bst_insert(root, a[i]);
    }
    return *root;
}

void
bst_pre_iterate(struct bst_node *root)
{
    if (root) {
        printf("%d ", root->data);
        bst_pre_iterate(root->left);
        bst_pre_iterate(root->right);
    }
}

void
bst_in_iterate(struct bst_node *root)
{
    if (root) {
        bst_in_iterate(root->left);
        printf("%d ", root->data);
        bst_in_iterate(root->right);
    }
}

void
bst_post_iterate(struct bst_node *root)
{
    if (root) {
        bst_post_iterate(root->left);
        bst_post_iterate(root->right);
        printf("%d ", root->data);
    }
}

struct bst_node *
bst_min(struct bst_node *root)
{
    struct bst_node *p = NULL;
    while (root) {
        p    = root;
        root = root->left;
    }
    return p;
}

struct bst_node *
bst_max(struct bst_node *root)
{
    struct bst_node *p = NULL;
    while (root) {
        p    = root;
        root = root->right;
    }
    return p;
}

struct bst_node *
bst_search(struct bst_node *root, datatype key)
{
    if (!root)
        return NULL;

    if (key == root->data)
        return root;
    else if (key > root->data)
        bst_search(root->right, key);
    else if (key < root->data)
        bst_search(root->left, key);
}

int
bst_delete(struct bst_node **root, datatype key)
{
    if (!*root)
        return -1;

    if (key > (*root)->data)
        return bst_delete(&(*root)->right, key);
    else if (key < (*root)->data)
        return bst_delete(&(*root)->left, key);
    else if ((*root)->left == NULL && (*root)->right == NULL) {
        free(*root);
        *root = NULL;
        return 0;
    } else if ((*root)->left != NULL && (*root)->right != NULL) {
        struct bst_node *p  = (*root)->left;
        struct bst_node *pp = NULL;
        while (p) {
            pp = p;
            p  = p->right;
        }
        (*root)->data = pp->data;
        return bst_delete(&(*root)->left, pp->data);
    } else {
        struct bst_node *p = *root;
        *root              = (*root)->left ? (*root)->left : (*root)->right;
        free(p);
        return 0;
    }
}
