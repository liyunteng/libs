/*
 * xmlcfg.c - xmlcfg
 *
 * Author : liyunteng <liyunteng@streamocean.com>
 * Date   : 2019/08/24
 *
 * Copyright (C) 2019 StreamOcean, Inc.
 * All rights reserved.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xmlcfg.h"

#ifndef ERR
#    define ERR(fmt, args...)                                                  \
        fprintf(stderr, "%s:L%d" fmt "\n", __FILE__, __LINE__, ##args)
#endif

xmlcfg_ptr
xmlcfg_init_bypath(const char *filepath)
{
    xmlDocPtr doc  = NULL;
    xmlcfg_ptr cfg = NULL;

    if (!filepath) {
        ERR("filepath is NULL!");
        goto err;
    }

    xmlInitParser();
    LIBXML_TEST_VERSION;

    doc = xmlParseFile(filepath);
    if (!doc) {
        ERR("Parse file %s failed!", filepath);
        goto err;
    }

    cfg = (xmlcfg_ptr)malloc(sizeof(xmlcfg_t));
    if (!cfg) {
        ERR("Out of memory!");
        goto err;
    }
    cfg->doc  = doc;
    cfg->root = xmlDocGetRootElement(doc);
    return cfg;

err:
    if (doc) {
        xmlFreeDoc(doc);
    }

    return NULL;
}

xmlcfg_ptr
xmlcfg_init_bymem(const char *buffer, int size)
{
    xmlDocPtr doc  = NULL;
    xmlcfg_ptr cfg = NULL;

    if (!buffer) {
        ERR("buffer is NULL!");
        goto err;
    }
    if (size <= 0) {
        ERR("size invalid! size=%d", size);
        goto err;
    }

    xmlInitParser();
    LIBXML_TEST_VERSION;

    doc = xmlParseMemory(buffer, size);
    if (!doc) {
        ERR("Parse memory failed!");
        goto err;
    }

    cfg = (xmlcfg_ptr)malloc(sizeof(xmlcfg_t));
    if (!cfg) {
        ERR("Out of memory");
        goto err;
    }
    cfg->doc  = doc;
    cfg->root = xmlDocGetRootElement(doc);
    return cfg;

err:
    if (doc) {
        xmlFreeDoc(doc);
    }
    return NULL;
}

void
xmlcfg_cleanup(xmlcfg_ptr cfg)
{
    if (cfg) {
        if (cfg->root) {
            xmlFreeDoc(cfg->doc);
        }
        free(cfg);
    }
}

static xmlNodePtr
search_node(xmlNodePtr root, const char *key)
{
    const char *p = strstr(key, KEY_TOKEN);
    const char *q = key;
    char buffer[MAX_KEY_LEN];
    xmlNodePtr node = NULL;

    if (*key == '\0') {
        return root;
    }

    if (p) {
        if (p - q > MAX_KEY_LEN) {
            ERR("KEY is too long to parse! key=%s", key);
            return NULL;
        }

        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, q, p - q);
        q = (const char *)buffer;
    }

    if (xmlStrcmp(root->name, (const xmlChar *)q) == 0) {
        node = root;
    } else {
        for (node = root->children; node; node = node->next) {
            if (node->type != XML_ELEMENT_NODE) {
                continue;
            }
            if (xmlStrcmp(node->name, (const xmlChar *)q)) {
                continue;
            } else {
                break;
            }
        }
    }

    if (p) {
        return search_node(node, p + 1);
    } else {
        return node;
    }

    return NULL;
}

int
xmlcfg_get_str(xmlcfg_ptr cfg, const char *key, const char **val)
{
    xmlNodePtr node = search_node(cfg->root, key);
    if (node) {
        xmlNodePtr text = node->children;
        for (; text; text = text->next) {
            if (text->type != XML_TEXT_NODE
                && text->type != XML_CDATA_SECTION_NODE) {
                continue;
            } else {
                *val = (const char *)text->content;
                return 0;
            }
        }
    }
    return -1;
}

int
xmlcfg_get_int(xmlcfg_ptr cfg, const char *key, int64_t *val)
{
    const char *p = NULL;
    int ret       = xmlcfg_get_str(cfg, key, &p);
    if (ret) {
        return ret;
    }

    char *q   = (char *)p;
    int64_t v = strtol(p, &q, 10);
    if (v == INT64_MIN || v == INT64_MAX || p == q) {
        return -1;
    } else {
        *val = v;
        return 0;
    }
}

int
xmlcfg_get_int32(xmlcfg_ptr cfg, const char *key, int32_t *val)
{
    const char *p = NULL;
    int ret       = xmlcfg_get_str(cfg, key, &p);
    if (ret) {
        return ret;
    }
#ifndef LONG_MAX
#    define LONG_MAX 2147483647
#endif
#ifndef LONG_MIN
#    define LONG_MIN (-LONG_MAX - 1)
#endif
    char *q   = (char *)p;
    int32_t v = strtol(p, &q, 10);
    if (v == LONG_MAX || v == LONG_MIN || p == q) {
        return -1;
    } else {
        *val = v;
        return 0;
    }
}

int
xmlcfg_get_size(xmlcfg_ptr cfg, const char *key, size_t *val)
{
    const char *p = NULL;
    int ret       = xmlcfg_get_str(cfg, key, &p);
    if (ret) {
        return ret;
    }

    char *q   = (char *)p;
    int64_t v = strtol(p, &q, 10);
    if (v == INT64_MIN || v == INT64_MAX || p == q) {
        return -1;
    }
    switch (*q) {
    case 'k':
    case 'K':
        v <<= 10;
        break;

    case 'm':
    case 'M':
        v <<= 20;
        break;

    case 'g':
    case 'G':
        v <<= 30;
        break;

    case 't':
    case 'T':
        v <<= 40;
        break;
    }

    *val = (size_t)v;
    return 0;
}

xmlcfg_ptr
xmlcfg_iter_init(xmlcfg_ptr cfg, const char *parent, const char *name)
{
    xmlcfg_ptr iter = (xmlcfg_ptr)malloc(sizeof(xmlcfg_t));
    if (!iter) {
        goto err;
    } else {
        memset(iter, 0, sizeof(xmlcfg_t));
        iter->name = name;
    }

    xmlNodePtr node = search_node(cfg->root, parent);
    if (!node) {
        goto err;
    }

    xmlNodePtr n = NULL;
    for (n = node->children; n; n = n->next) {
        if (n->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcmp(n->name, (const xmlChar *)name) == 0) {
            iter->root = n;
            break;
        }
    }
    if (iter->root) {
        return iter;
    }

err:
    if (iter) {
        free(iter);
    }
    return NULL;
}

int
xmlcfg_iter_hasnext(xmlcfg_ptr iter)
{
    if (iter) {
        return iter->root != NULL;
    } else {
        return 0;
    }
}

xmlcfg_ptr
xmlcfg_iter_next(xmlcfg_ptr iter)
{
    xmlNodePtr n = iter->root->next;
    for (; n; n = n->next) {
        if (n->type != XML_ELEMENT_NODE) {
            continue;
        }
        if (xmlStrcmp(n->name, (const xmlChar *)iter->name) == 0) {
            iter->root = n;
            return iter;
        }
    }

    iter->root = NULL;
    return iter;
}
