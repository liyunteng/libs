/*
 * libconfig.h - libconfig
 *
 * Date   : 2020/04/25
 */
#ifndef __LIBCONFIG_H__
#define __LIBCONFIG_H__

#include <json-c/json.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct json_object cfg_ctx_t;
typedef void (*cfg_prop_iter)(char *key, char *data, void *priv_data);

cfg_ctx_t *cfg_create(void);
cfg_ctx_t *cfg_create_from_file(const char *filename);
int cfg_write_to_file(cfg_ctx_t *cfg, const char *filename);
void cfg_destroy(cfg_ctx_t *cfg);

char *cfg_get(cfg_ctx_t *cfg, const char *key);
int cfg_set(cfg_ctx_t *cfg, const char *key, const char *data);
void cfg_remove(cfg_ctx_t *cfg, const char *key);

void cfg_iter(cfg_ctx_t *cfg, cfg_prop_iter, void *priv_data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
