/*
 * libconfig.c - libconfig
 *
 * Date   : 2020/04/25
 */
#include "libconfig.h"

#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#define CFG_PROP_LINE_MAX_LEN 512

typedef enum {
    PROP_STATE_SPACE,
    PROP_STATE_SHARP,
    PROP_STATE_KEY,
    PROP_STATE_WAIT_EQUAL,
    PROP_STATE_WAIT_VALUE,
    PROP_STATE_VALUE,
    PROP_STATE_LINEBREAK,
    PROP_STATE_END,

    PROP_STATE_LAST
} CFG_PROP_STATE;


void cfg_prop_open(cfg_prop_obj_t **cfg_obj)
{
    *cfg_obj = json_object_new_object();
}


void
cfg_prop_open_from_file(char *filename, cfg_prop_obj_t **cfg_obj)
{
    FILE *fp = NULL;
    char cfg_line[CFG_PROP_LINE_MAX_LEN] = {0};
    char cfg_key[CFG_PROP_LINE_MAX_LEN], cfg_value[CFG_PROP_LINE_MAX_LEN];
    int32_t i_key = 0, i_value = 0;
    char *datap = NULL;
    cfg_prop_obj_t *cfg_obj_p;
    CFG_PROP_STATE state, state_last;

    if ((fp = fopen(filename, "r")) == NULL) {
        *cfg_obj = NULL;
        return;
    }

    cfg_obj_p = json_object_new_object();
    while (fgets(cfg_line, CFG_PROP_LINE_MAX_LEN - 1, fp) != NULL) {
        datap = cfg_line;
        state = PROP_STATE_SPACE;
        state_last = PROP_STATE_SPACE;

        while (state != PROP_STATE_END && datap != NULL) {
            switch (state) {
            case PROP_STATE_SPACE:
                if (*datap != ' ' && *datap != '\t') {
                    if (*datap == '#') {
                        state_last = state;
                        state = PROP_STATE_SHARP;
                    } else if (*datap == '\n') {
                        state_last = state;
                        state = PROP_STATE_LINEBREAK;
                    } else {
                        memset(cfg_key, 0, sizeof(cfg_key));
                        i_key = 0;
                        cfg_key[i_key ++] = *datap;

                        state_last = state;
                        state = PROP_STATE_KEY;
                    }
                }
                datap ++;
                break;
            case PROP_STATE_SHARP:
                state_last = state;
                state = PROP_STATE_LINEBREAK;
                datap ++;
                break;
            case PROP_STATE_KEY:
                if (*datap == '\n') {
                    state_last = state;
                    state = PROP_STATE_LINEBREAK;
                } else if (*datap == '=') {
                    state_last = state;
                    state = PROP_STATE_WAIT_VALUE;
                } else if (*datap == ' ' || *datap == '\t') {
                    state_last = state;
                    state = PROP_STATE_WAIT_EQUAL;
                } else {
                    cfg_key[i_key ++] = *datap;
                }
                datap ++;
                break;
            case PROP_STATE_WAIT_EQUAL:
                if (*datap != ' ' && *datap != '\t') {
                    if (*datap == '=') {
                        state_last = state;
                        state = PROP_STATE_WAIT_VALUE;
                    } else {
                        state_last = state;
                        state = PROP_STATE_LINEBREAK;
                    }
                }
                datap ++;
                break;

            case PROP_STATE_WAIT_VALUE:
                if (*datap != ' ' && *datap != '\t') {
                    if (*datap == '\n') {
                        state_last = state;
                        state = PROP_STATE_LINEBREAK;
                    } else {
                        memset(cfg_value, 0, sizeof(cfg_value));
                        i_value = 0;
                        cfg_value[i_value ++] = *datap;

                        state_last = state;
                        state = PROP_STATE_VALUE;
                    }
                }
                datap ++;
                break;

            case PROP_STATE_VALUE:
                if (*datap == '\n') {
                    state_last = state;
                    state = PROP_STATE_LINEBREAK;
                } else {
                    cfg_value[i_value ++] = *datap;
                }
                datap ++;
                break;

            case PROP_STATE_LINEBREAK:
                if (state_last == PROP_STATE_SPACE) {

                } else if (state_last == PROP_STATE_SHARP) {

                } else if (state_last == PROP_STATE_KEY ||
                           state_last == PROP_STATE_WAIT_EQUAL ||
                           state_last == PROP_STATE_WAIT_VALUE) {

                } else if (state_last == PROP_STATE_VALUE) {
                    cfg_key[i_key] = '\0';
                    cfg_value[i_value] = '\0';
                    json_object_object_add(cfg_obj_p, cfg_key, json_object_new_string(cfg_value));
                }
                state_last = state;
                state = PROP_STATE_END;
                datap ++;
                break;
            default:
                datap ++;
                break;
            }
        }
    }

    fclose(fp);
    *cfg_obj = cfg_obj_p;
}

static void
cfg_prop_write_to_file_local(char *key, char *data, void *priv_data)
{
    FILE *fp = priv_data;
    fprintf(fp, "%s=%s\n", key, data);
}

int
cfg_prop_write_to_file(char *filename, cfg_prop_obj_t *cfg_obj)
{
    FILE *fp = NULL;
    if ((fp = fopen(filename, "w")) == NULL) {
        return -1;
    }

    cfg_prop_iter(cfg_obj, cfg_prop_write_to_file_local, fp);

    fclose(fp);
    return 0;
}

void
cfg_prop_close(cfg_prop_obj_t *cfg_obj)
{
    json_object_put(cfg_obj);
    cfg_obj = NULL;
}

char *
cfg_prop_get(cfg_prop_obj_t *cfg_obj, char *key)
{
    struct json_object *val = json_object_object_get(cfg_obj, key);
    if (!val)  {
        return NULL;
    }
    return (char *)json_object_get_string(val);
}

void
cfg_prop_set(cfg_prop_obj_t *cfg_obj, char *key, char *data)
{
    json_object_object_add(cfg_obj, key, json_object_new_string(data));
}

void
cfg_prop_remove(cfg_prop_obj_t *cfg_obj, char *key)
{
    json_object_object_del(cfg_obj, key);
}

void
cfg_prop_iter(cfg_prop_obj_t *cfg_obj, cfg_prop_iter_handler handler, void *priv_data)
{
    char *key;
    struct json_object *val;
    struct lh_entry *entry;

    entry = json_object_get_object(cfg_obj)->head;
    while (entry) {
        key = (char *)entry->k;
        val = (struct json_object*)entry->v;
        entry = entry->next;

        handler(key, (char *)json_object_get_string(val), priv_data);
    }
}
