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

int main(void)
{
    cfg_prop_obj_t *cfg_obj = NULL;
    char *filename = "test_config.cfg";

    cfg_prop_open_from_file(filename, &cfg_obj);
    cfg_prop_iter(cfg_obj, print, NULL);

    char *url = cfg_prop_get(cfg_obj, "url");
    printf("%s\n", url);
    cfg_prop_set(cfg_obj, "abc", "1234");
    cfg_prop_remove(cfg_obj, "a");

    cfg_prop_write_to_file(filename, cfg_obj);

    cfg_prop_close(cfg_obj);
    return 0;
}
