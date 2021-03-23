/*
 * libconfig.c - libconfig
 *
 * Date   : 2020/04/25
 */
#include "libconfig.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#define LINE_SIZE 512

typedef enum {
    PARSE_STATE_SPACE,
    PARSE_STATE_SHARP,
    PARSE_STATE_KEY,
    PARSE_STATE_WAIT_EQUAL,
    PARSE_STATE_WAIT_VALUE,
    PARSE_STATE_VALUE,
    PARSE_STATE_LINEBREAK,
    PARSE_STATE_END,

    PARSE_STATE_LAST
} CFG_PARSE_STATE;

cfg_ctx_t *
cfg_create(void)
{
    cfg_ctx_t *cfg = json_object_new_object();
    return cfg;
}

cfg_ctx_t *
cfg_create_from_file(const char *filename)
{
    FILE *fp              = NULL;
    cfg_ctx_t *cfg        = NULL;
    char line[LINE_SIZE]  = {0};
    char key[LINE_SIZE]   = {0};
    char value[LINE_SIZE] = {0};
    char *datap           = NULL;
    int i_key, i_value;
    CFG_PARSE_STATE state, last_state;


    fp = fopen(filename, "r");
    if (fp == NULL) {
        goto failed;
    }

    cfg = json_object_new_object();
    if (!cfg) {
        goto failed;
    }

    while (fgets(line, LINE_SIZE - 1, fp) != NULL) {
        datap      = line;
        state      = PARSE_STATE_SPACE;
        last_state = PARSE_STATE_SPACE;

        while (state != PARSE_STATE_END && datap != NULL) {
            switch (state) {
            case PARSE_STATE_SPACE: {
                if (*datap != ' ' && *datap != '\t') {
                    if (*datap == '#') {
                        last_state = state;
                        state      = PARSE_STATE_SHARP;
                    } else if (*datap == '\n') {
                        last_state = state;
                        state      = PARSE_STATE_LINEBREAK;
                    } else {
                        memset(key, 0, LINE_SIZE);
                        i_key        = 0;
                        key[i_key++] = *datap;

                        last_state = state;
                        state      = PARSE_STATE_KEY;
                    }
                }
                datap++;
                break;
            }
            case PARSE_STATE_SHARP: {
                last_state = state;
                state      = PARSE_STATE_LINEBREAK;
                datap++;
                break;
            }
            case PARSE_STATE_KEY: {
                if (*datap == '\n') {
                    last_state = state;
                    state      = PARSE_STATE_LINEBREAK;
                } else if (*datap == '=') {
                    last_state = state;
                    state      = PARSE_STATE_WAIT_VALUE;
                } else if (*datap == ' ' || *datap == '\t') {
                    last_state = state;
                    state      = PARSE_STATE_WAIT_EQUAL;
                } else {
                    key[i_key++] = *datap;
                }
                datap++;
                break;
            }
            case PARSE_STATE_WAIT_EQUAL: {
                if (*datap != ' ' && *datap != '\t') {
                    if (*datap == '=') {
                        last_state = state;
                        state      = PARSE_STATE_WAIT_VALUE;
                    } else {
                        last_state = state;
                        state      = PARSE_STATE_LINEBREAK;
                    }
                }
                datap++;
                break;
            }
            case PARSE_STATE_WAIT_VALUE: {
                if (*datap != ' ' && *datap != '\t') {
                    if (*datap == '\n') {
                        last_state = state;
                        state      = PARSE_STATE_LINEBREAK;
                    } else {
                        memset(value, 0, LINE_SIZE);
                        i_value          = 0;
                        value[i_value++] = *datap;

                        last_state = state;
                        state      = PARSE_STATE_VALUE;
                    }
                }
                datap++;
                break;
            }
            case PARSE_STATE_VALUE: {
                if (*datap == '\n') {
                    last_state = state;
                    state      = PARSE_STATE_LINEBREAK;
                } else {
                    value[i_value++] = *datap;
                }
                datap++;
                break;
            }
            case PARSE_STATE_LINEBREAK: {
                if (last_state == PARSE_STATE_SPACE) {
                } else if (last_state == PARSE_STATE_SHARP) {
                } else if (last_state == PARSE_STATE_KEY
                           || last_state == PARSE_STATE_WAIT_EQUAL
                           || last_state == PARSE_STATE_WAIT_VALUE) {
                } else if (last_state == PARSE_STATE_VALUE) {
                    key[i_key]     = '\0';
                    value[i_value] = '\0';
                    json_object_object_add(
                        cfg, key, json_object_new_string(value));
                }

                last_state = state;
                state      = PARSE_STATE_END;
                datap++;
                break;
            }
            default:
                datap++;
                break;
            }
        }
    }

    fclose(fp);
    fp = NULL;
    return cfg;


failed:
    if (fp) {
        fclose(fp);
        fp = NULL;
    }
    if (cfg) {
        json_object_put(cfg);
        cfg = NULL;
    }
    return NULL;
}


static void
cfg_do_write_to_file(char *key, char *data, void *priv_data)
{
    FILE *fp = priv_data;
    if (fp) {
        fprintf(fp, "%s=%s\n", key, data);
    }
}

int
cfg_write_to_file(cfg_ctx_t *cfg, const char *filename)
{
    assert(cfg);
    assert(filename);
    FILE *fp = NULL;
    if ((fp = fopen(filename, "w")) == NULL) {
        return -1;
    }

    cfg_iter(cfg, cfg_do_write_to_file, fp);

    fclose(fp);
    return 0;
}

void
cfg_destroy(cfg_ctx_t *cfg)
{
    if (cfg) {
        json_object_put(cfg);
        cfg = NULL;
    }
}

char *
cfg_get(cfg_ctx_t *cfg, const char *key)
{
    assert(cfg);
    assert(key);
    struct json_object *val = json_object_object_get(cfg, key);
    if (!val) {
        return NULL;
    }
    return (char *)json_object_get_string(val);
}

int
cfg_set(cfg_ctx_t *cfg, const char *key, const char *data)
{
    return json_object_object_add(cfg, key, json_object_new_string(data));
}

void
cfg_remove(cfg_ctx_t *cfg, const char *key)
{
    json_object_object_del(cfg, key);
}

void
cfg_iter(cfg_ctx_t *cfg, cfg_prop_iter handler, void *priv_data)
{
    char *key;
    struct json_object *val;
    struct lh_entry *entry;

    entry = json_object_get_object(cfg)->head;
    while (entry) {
        key   = (char *)entry->k;
        val   = (struct json_object *)entry->v;
        entry = entry->next;

        handler(key, (char *)json_object_get_string(val), priv_data);
    }
}
