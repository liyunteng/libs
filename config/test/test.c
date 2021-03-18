/*
 * test_config.c - test_config
 *
 * Date   : 2020/04/25
 */
#include <stdio.h>

#include "libconfig.h"

static void print(char *key, char *data, void *priv_data)
{
    printf("%s = %s\n", key, data);
}

int libconfig_test(void)
{
    cfg_prop_obj_t *cfg_obj = NULL;
    char *filename = "test_config.cfg";

    cfg_prop_open_from_file(filename, &cfg_obj);
    cfg_prop_iter(cfg_obj, print, NULL);


    char *url = cfg_prop_get(cfg_obj, "url");
    if (url) {
        printf("url: %s\n", url);
    }

    char *abc = cfg_prop_get(cfg_obj, "abc");
    if (abc) {
        printf("abc: %s\n", abc);
    } else {
        cfg_prop_set(cfg_obj, "abc", "1234");
    }

    char *cde = cfg_prop_get(cfg_obj, "cde");
    if (cde) {
        cfg_prop_remove(cfg_obj, "cde");
    } else {
        cfg_prop_set(cfg_obj, "cde", "3333");
    }

    cfg_prop_write_to_file(filename, cfg_obj);

    cfg_prop_close(cfg_obj);

    return 0;
}

int main(void)
{
    libconfig_test();
    return 0;
}
