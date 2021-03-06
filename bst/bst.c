/*
 * bst.c - bst
 *
 * Date   : 2021/03/18
 */
#include "bst.h"
#include <stdio.h>

void
bst_destroy(Node **root)
{
    if (!root)
        return;

    if ((*root)->lchild)
        bst_destroy(&(*root)->lchild);
    if ((*root)->rchild)
        bst_destroy(&(*root)->rchild);

    if (!(*root)->lchild && !(*root)->rchild) {
        free(*root);
        *root = NULL;
    }

    return;
}

static Node *
__bst_insert(Node **root, Node *pre, datatype data)
{
    if (!*root) {
        *root = (Node *)malloc(sizeof(Node));
        if (!*root) {
            fprintf(stderr, "mallo failed.");
            return NULL;
        }
        (*root)->data   = data;
        (*root)->lchild = (*root)->rchild = NULL;
        (*root)->parent                   = pre;
        if (pre) {
            if (data > pre->data)
                pre->rchild = *root;
            else
                pre->lchild = *root;
        }
        return *root;
    }

    if (data > (*root)->data)
        __bst_insert(&(*root)->rchild, *root, data);
    else
        __bst_insert(&(*root)->lchild, *root, data);

    return NULL;
}

Node *
bst_insert(Node **root, datatype data)
{
    if (!root)
        return NULL;

    return __bst_insert(root, NULL, data);
}

Node *
bst_create(Node **root, datatype arr[], size_t len)
{
    size_t i;
    for (i = 0; i < len; i++) {
        bst_insert(root, arr[i]);
    }
    return *root;
}

void
bst_pre_iterate(Node *root)
{
    if (root) {
        printf("%d ", root->data);
        bst_pre_iterate(root->lchild);
        bst_pre_iterate(root->rchild);
    }
}

void
bst_in_iterate(Node *root)
{
    if (root) {
        bst_in_iterate(root->lchild);
        printf("%d ", root->data);
        bst_in_iterate(root->rchild);
    }
}

void
bst_post_iterate(Node *root)
{
    if (root) {
        bst_post_iterate(root->lchild);
        bst_post_iterate(root->rchild);
        printf("%d ", root->data);
    }
}
Node *
bst_min(Node *root)
{
    Node *p = NULL;
    while (root) {
        p    = root;
        root = root->lchild;
    }

    return p;
}
Node *
bst_max(Node *root)
{
    Node *p = NULL;
    while (root) {
        p    = root;
        root = root->rchild;
    }

    return p;
}
Node *
bst_search(Node *root, datatype data)
{
    while (root) {
        if (data == root->data)
            break;
        else if (data > root->data)
            root = root->rchild;
        else
            root = root->lchild;
    }
    return root;
}

int
bst_delete(Node *root, datatype data)
{
    if (!root)
        return -1;

    if (data > root->data)

        return bst_delete(root->rchild, data);
    else if (data < root->data)
        return bst_delete(root->lchild, data);
    else if (root->lchild == NULL && root->rchild == NULL) {
        Node *parent = root->parent;
        if (parent) {
            if (parent->lchild == root)
                parent->lchild = NULL;
            else
                parent->rchild = NULL;
        }
        free(root);
        return 0;
    } else if (root->lchild != NULL && root->rchild != NULL) {
        Node *p   = root->lchild;
        Node *one = NULL;
        while (p) {
            one = p;
            p   = p->rchild;
        }

        root->data = one->data;
        return bst_delete(root->lchild, one->data);
    } else {
        if (root->lchild) {
            root->data = root->lchild->data;
            return bst_delete(root->lchild, root->data);
        } else {
            root->data = root->rchild->data;
            return bst_delete(root->rchild, root->data);
        }
    }
    return -2;
}
