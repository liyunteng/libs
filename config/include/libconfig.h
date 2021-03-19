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

typedef struct json_object cfg_prop_obj_t;
typedef void(*cfg_prop_iter_handler) (char *key, char *data, void *priv_data);

void cfg_prop_open(cfg_prop_obj_t **cfg_obj);

void cfg_prop_open_from_file(char *filename, cfg_prop_obj_t **cfg_obj);

int cfg_prop_write_to_file(char *filename, cfg_prop_obj_t *cfg_obj);

void cfg_prop_close(cfg_prop_obj_t *cfg_obj);

char * cfg_prop_get(cfg_prop_obj_t *cfg_obj, char *key);

void cfg_prop_set(cfg_prop_obj_t *cfg_obj, char *key, char *data);

void cfg_prop_remove(cfg_prop_obj_t *cfg_obj, char *key);

void cfg_prop_iter(cfg_prop_obj_t *cfg_obj, cfg_prop_iter_handler, void *priv_data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
