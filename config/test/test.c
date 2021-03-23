/*
 * test_config.c - test_config
 *
 * Date   : 2020/04/25
 */
#include "libconfig.h"

#include <assert.h>
#include <stdio.h>

static void
print(char *key, char *data, void *priv_data)
{
    printf("%s = %s\n", key, data);
}

int
libconfig_test(void)
{
    cfg_ctx_t *cfg = NULL;
    char *filename = "test_config.cfg";

    cfg = cfg_create_from_file(filename);
    assert(cfg);

    cfg_iter(cfg, print, NULL);
    printf("============\n");

    char *url = cfg_get(cfg, "url");
    if (url) {
        printf("url: %s\n", url);
    }

    char *abc = cfg_get(cfg, "abc");
    if (abc) {
        printf("abc: %s\n", abc);
    } else {
        cfg_set(cfg, "abc", "1234");
    }

    char *cde = cfg_get(cfg, "cde");
    if (cde) {
        cfg_remove(cfg, "cde");
    } else {
        cfg_set(cfg, "cde", "3333");
    }

    cfg_write_to_file(cfg, filename);

    cfg_destroy(cfg);

    return 0;
}

int
main(void)
{
    libconfig_test();
    return 0;
}
