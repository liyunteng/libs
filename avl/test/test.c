/*
 * avl_test.c - avl_test
 *
 * Author : liyunteng <liyunteng@streamocean.com>
 * Date   : 2019/09/03
 *
 * Copyright (C) 2019 StreamOcean, Inc.
 * All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "avl.h"

avl_tree_h_td my_avl_tree;

typedef struct my_object {
    char *key;
} my_object_t;

void *
my_malloc_fn(size_t size)
{
    return (malloc(size));
}

void
my_free_fn(void *mem_ptr)
{
    free(mem_ptr);
}

void
my_cleanup_fn(void *mem_ptr)
{
    my_object_t *obj = (my_object_t *)mem_ptr;
    free(obj->key);
}

avl_tree_compare_code_td
my_compare_fn(void *obj1, void *obj2)
{
    avl_tree_compare_code_td rc = AVL_TREE_EQ;
    int compare;
    compare = strcmp(((my_object_t *)obj1)->key, ((my_object_t *)obj2)->key);

    if (compare > 0) {
        rc = AVL_TREE_GT;
    } else if (compare < 0) {
        rc = AVL_TREE_LT;
    }
    return rc;
}

int
my_object_sanity_fn(void *obj)
{
    return 0;
}

avl_tree_walk_code_td
my_walk_fn(void *obj, void *ctx)
{
    (void)ctx;
    my_object_t *local_obj = (my_object_t *)obj;
    printf("Walk %s\n", local_obj->key);
    return (AVL_CONT_WALK);
}

int
main(int argc, char *argv[])
{
    int rc = 0;
    time_t showtime;
    my_object_t *obj;
    my_object_t *found_obj = NULL;
    avl_tree_walk_code_td walk_result;
    avl_tree_search_options_td search_result;

    if (argc != 2) {
        showtime = time(NULL);
        printf("\n%.15s: Usage: %s sleeptime\n", ctime(&showtime) + 4, argv[0]);
        rc = -1;
    } else {
        sleep(atoi(argv[argc - 1]));
        rc =
            avl_tree_init(my_compare_fn, AVL_TREE_OPTION_DEFAULT, &my_avl_tree);
        if (rc == 0) {
            showtime = time(NULL);
            printf("INIT AVL SUCCESS\n");
        }

        if (rc == 0) {
            rc = avl_tree_init_fns(my_malloc_fn, my_free_fn, my_cleanup_fn,
                                   my_object_sanity_fn, my_avl_tree);
        }

        if (rc == 0) {
            showtime = time(NULL);
            printf("INIT FNS AVL SUCCESS\n");
        }

        if (rc == 0) {
            rc = avl_tree_allocate_object(my_avl_tree, (void *)&obj,
                                          sizeof(my_object_t));
        }
        if (rc == 0) {
            obj->key = strdup("key1");
            printf("key = %s\n", obj->key);
            printf("address = %p\n", obj);
        }

        if (rc == 0) {
            rc = avl_tree_insert(my_avl_tree, obj, NULL);
        }

        if (rc == 0) {
            showtime = time(NULL);
            printf("INSERT AVL SUCCESS\n");
        }

        if (rc == 0) {
            rc = avl_tree_allocate_object(my_avl_tree, (void *)&obj,
                                          sizeof(my_object_t));
        }

        if (rc == 0) {
            obj->key = strdup("key2");
            printf("key = %s\n", obj->key);
        }

        if (rc == 0) {
            rc = avl_tree_insert(my_avl_tree, obj, (void *)&found_obj);
            if (found_obj) {
                printf("key = %s\n", found_obj->key);
                printf("address = %p\n", found_obj);
            }
        }

        if (rc == 0) {
            showtime = time(NULL);
            printf("INSERT AVL SUCCESS\n");
        }

        if (rc == 0) {
            rc = avl_tree_allocate_object(my_avl_tree, (void *)&obj,
                                          sizeof(my_object_t));
        }
        if (rc == 0) {
            obj->key = strdup("key3");
            printf("key = %s\n", obj->key);
        }

        if (rc == 0) {
            rc = avl_tree_insert(my_avl_tree, obj, NULL);
        }

        if (rc == 0) {
            showtime = time(NULL);
            printf("INSERT AVL SUCCESS\n");
        }

        if (rc == 0) {
            rc = avl_tree_walk(my_avl_tree, my_walk_fn, NULL, &walk_result);
            if (rc == 0) {
                showtime = time(NULL);
                printf("WALK AVL SUCCESS\n");
            }
        }

        if (rc == 0) {
            rc = avl_tree_search(my_avl_tree, AVL_TREE_OPTION_MIN, NULL,
                                 (void *)&found_obj, &search_result);
            if (rc == 0) {
                printf("MIN key = %s\n", found_obj->key);
            }
        }

        if (rc == 0) {
            rc = avl_tree_search(my_avl_tree, AVL_TREE_OPTION_MAX, NULL,
                                 (void *)&found_obj, &search_result);
            if (rc == 0) {
                printf("MAX key = %s\n", found_obj->key);
            }
        }

        if (rc == 0) {
            rc = avl_tree_allocate_object(my_avl_tree, (void *)&obj,
                                          sizeof(my_object_t));
        }
        if (rc == 0) {
            obj->key = strdup("key2");
            printf("key = %s\n", obj->key);

            rc = avl_tree_search(my_avl_tree, AVL_TREE_OPTION_EQ, obj,
                                 (void *)&found_obj, &search_result);
            if (rc == 0) {
                printf("EQ key = %s\n", found_obj->key);
            }
        }

        if (rc == 0) {
            printf("key = %s\n", obj->key);
            rc = avl_tree_search(my_avl_tree, AVL_TREE_OPTION_GT, obj,
                                 (void *)&found_obj, &search_result);
            if (rc == 0) {
                printf("GT key = %s\n", found_obj->key);
            }
        }

        if (rc == 0) {
            printf("key = %s\n", obj->key);

            rc = avl_tree_search(my_avl_tree, AVL_TREE_OPTION_GE, obj,
                                 (void *)&found_obj, &search_result);
            if (rc == 0) {
                if ((search_result & AVL_TREE_OPTION_EQ)
                    == AVL_TREE_OPTION_EQ) {
                    printf("GE -equal key = %s\n", found_obj->key);
                } else {
                    printf("GE -greater key = %s\n", found_obj->key);
                }
            }
        }

        if (rc == 0) {
            strcpy(obj->key, "key0");
            printf("key = %s\n", obj->key);

            rc = avl_tree_search(my_avl_tree, AVL_TREE_OPTION_GE, obj,
                                 (void *)&found_obj, &search_result);
            if (rc == 0) {
                if ((search_result & AVL_TREE_OPTION_EQ)
                    == AVL_TREE_OPTION_EQ) {
                    printf("GE -equal key = %s\n", found_obj->key);
                } else {
                    printf("GE -granter key = %s\n", found_obj->key);
                }
            }
        }

        if (rc == 0) {
            strcpy(obj->key, "key2");
            printf("key = %s\n", obj->key);

            rc = avl_tree_search(my_avl_tree, AVL_TREE_OPTION_LT, obj,
                                 (void *)&found_obj, &search_result);
            if (rc == 0) {
                printf("LT key = %s\n", found_obj->key);
            }
        }

        if (rc == 0) {
            printf("key = %s\n", obj->key);

            rc = avl_tree_search(my_avl_tree, AVL_TREE_OPTION_LE, obj,
                                 (void *)&found_obj, &search_result);
            if (rc == 0) {
                if ((search_result & AVL_TREE_OPTION_EQ)
                    == AVL_TREE_OPTION_EQ) {
                    printf("LE -equal key = %s\n", found_obj->key);
                } else {
                    printf("LE -less key = %s\n", found_obj->key);
                }
            }
        }

        if (rc == 0) {
            strcpy(obj->key, "key4");
            printf("key = %s\n", obj->key);

            rc = avl_tree_search(my_avl_tree, AVL_TREE_OPTION_LE, obj,
                                 (void *)&found_obj, &search_result);
            if (rc == 0) {
                if ((search_result & AVL_TREE_OPTION_EQ)
                    == AVL_TREE_OPTION_EQ) {
                    printf("LE -equal key = %s\n", found_obj->key);
                } else {
                    printf("LE -less key = %s\n", found_obj->key);
                }
            }
        }

        if (rc == 0) {
            strcpy(obj->key, "key2");
            printf("key = %s\n", obj->key);
            rc = avl_tree_delete(my_avl_tree, obj, NULL);
            if (rc == 0) {
                showtime = time(NULL);
                printf("DELETE AVL SUCCESS\n");
            }
        }

        if (rc == 0) {
            rc = avl_tree_walk(my_avl_tree, my_walk_fn, NULL, &walk_result);
            if (rc == 0) {
                showtime = time(NULL);
                printf("WALK AVL SUCCESS\n");
            }
        }

        if (rc == 0) {
            rc = avl_tree_shutdown(&my_avl_tree, AVL_FREE_OBJECTS);
        }

        if (rc == 0) {
            printf("SHUTDOWN AVL SUCCESS\n");
        }
    }
    printf("Return Code = %d\n", rc);
    return 0;
}
