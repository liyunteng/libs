/*
 * test.c - test
 *
 * Date   : 2020/04/29
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <time.h>

#include "sbox.h"
#if 0
void walk(sbox_ptr dict) {
    sbox_kv_pair *pair = NULL;
    for(pair = sbox_dict_first(dict); pair; pair = sbox_dict_next(pair)) {
        printf("%s = %ld\n", sbox_get_str(pair->key), sbox_get_int(pair->value));
    }
}

void walk_list(sbox_ptr list) {
    sbox_list_node* node = NULL;
    int i = 0;
    for(node = sbox_list_first(list); node; node = sbox_list_next(node)) {
        printf("%d:%s\n", i++, sbox_get_str(node->data));
    }
}
#endif

void walk(sbox_ptr box) {
    char buff[4096];
    memset(buff, 0, sizeof(buff));
    sbox_tostr(box, buff);
    printf("%s\n", buff);

}



void node_print(int deep, struct rb_node *node)
{
    sbox_kv_pair *pair = NULL;
    int i;
    for(i = 0; i < deep; i++) {
        printf("----");
    }
    if(node) {
        pair = rb_entry(node, sbox_kv_pair, node);
        if(rb_is_red(node)) {
            printf("(R)");
        } else if(rb_is_black(node)) {
            printf("(B)");
        }
        printf("%s\n", sbox_get_str(pair->key));
        node_print(deep + 1, node->rb_left);
        node_print(deep + 1, node->rb_right);
    } else {
        printf("NULL\n");
    }
}

void dict_test(void)
{
    sbox_ptr dict = NULL;
    dict = sbox_new_dict();

    sbox_dict_insert(dict, sbox_new_str("1"), sbox_new_int(1));
    node_print(1, dict->d.root.rb_node);
    sbox_dict_insert(dict, sbox_new_str("2"), sbox_new_int(2));
    node_print(1, dict->d.root.rb_node);
    sbox_dict_insert(dict, sbox_new_str("3"), sbox_new_int(3));
    node_print(1, dict->d.root.rb_node);
    sbox_dict_insert(dict, sbox_new_str("4"), sbox_new_int(4));
    node_print(1, dict->d.root.rb_node);

    walk(dict);

    printf("Delete 3\n");
    sbox_dict_del(dict, sbox_new_str("3"));
    node_print(1, dict->d.root.rb_node);
    walk(dict);

    sbox_release(dict);
}

void list_test(void)
{
    sbox_ptr list = sbox_new_list();
    sbox_ptr p = NULL;
    sbox_list_append(list, sbox_new_str("a"));
    sbox_list_append(list, sbox_new_str("b"));
    sbox_list_append(list, sbox_new_str("c"));
    sbox_list_append(list, sbox_new_str("d"));

    walk(list);

    p = sbox_list_get(list, 0);
    printf("p is %s\n", sbox_get_str(p));
    sbox_list_del(list, p);

    walk(list);

    sbox_release(list);

}

int main(int argc, char* argv[])
{
    dict_test();

#if 0
    while(1) {
        dict_test();
        usleep(100);
    }
#endif

    list_test();

#if 0
    while(1) {
        list_test();
        usleep(200);
    }
#endif

    return 0;

}
