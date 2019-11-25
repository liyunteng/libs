/*
 * xmlcfg.h - xmlcfg
 *
 * Author : liyunteng <liyunteng@streamocean.com>
 * Date   : 2019/08/24
 *
 * Copyright (C) 2019 StreamOcean, Inc.
 * All rights reserved.
 */
#ifndef XMLCFG_H
#define XMLCFG_H

#define MAX_KEY_LEN 32
#define KEY_TOKEN "."

#include <libxml/parser.h>
#include <libxml/tree.h>

typedef struct {
    xmlDocPtr   doc;
    xmlNodePtr  root;
    const char *name;
} xmlcfg_t, *xmlcfg_ptr;

extern xmlcfg_ptr xmlcfg_init_bypath(const char *filepath);
extern xmlcfg_ptr xmlcfg_init_bymem(const char *buffer, int size);
extern void       xmlcfg_cleanup(xmlcfg_ptr xmlcfg);

extern int xmlcfg_get_str(xmlcfg_ptr cfg, const char *key, const char **val);
extern int xmlcfg_get_int(xmlcfg_ptr cfg, const char *key, int64_t *val);
extern int xmlcfg_get_size(xmlcfg_ptr cfg, const char *key, size_t *val);
extern int xmlcfg_get_int32(xmlcfg_ptr cfg, const char *key, int32_t *val);

extern xmlcfg_ptr xmlcfg_iter_init(xmlcfg_ptr cfg, const char *parent, const char *name);
extern int        xmlcfg_iter_hasnext(xmlcfg_ptr iter);
extern xmlcfg_ptr xmlcfg_iter_next(xmlcfg_ptr iter);
#endif
