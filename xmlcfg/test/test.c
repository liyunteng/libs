/*
 * test.c - test
 *
 * Author : liyunteng <liyunteng@streamocean.com>
 * Date   : 2019/08/24
 *
 * Copyright (C) 2019 StreamOcean, Inc.
 * All rights reserved.
 */
#include <string.h>
#include "xmlcfg.h"
#define ERR(fmt, args...) fprintf(stderr, fmt, ##args)
const char *xml_template = "                                        \
<root>                                                              \
    <serverid>1</serverid>                                          \
    <reporter>http://172.16.1.152:12345</reporter>                  \
    <upload protocol=\"full\">                                      \
        <content>                                                   \
            <ctype>live</ctype>                                     \
            <guid>live-test</guid>                                  \
            <priority>5</priority>                                  \
            <title>live test</title>                                \
            <description />                                         \
            <files>                                                 \
                <file>                                              \
                    <format>ts</format>                             \
                    <server>TSS</server>                            \
                    <name> http://172.16.1.217/live/vc-test </name> \
                    <codec>x264</codec>                             \
                    <bitrate>1024</bitrate>                         \
                    <streamsense>0</streamsense>                    \
                    <resolution>8</resolution>                      \
                    <!-- <timeshift>-1</timeshift> -->              \
                    <size>32k</size>                                \
                </file>                                             \
            </files>                                                \
        </content>                                                  \
    </upload>                                                       \
    <task_uuid>80</task_uuid>                                       \
</root>                                                             \
";

int test1()
{
    xmlcfg_ptr cfg = NULL;
    cfg= xmlcfg_init_bymem(xml_template, strlen(xml_template));
    if (cfg == NULL) {
        ERR("xmlcfg_init_byemem");
        return -1;
    }
    const char *p = NULL;
    if (xmlcfg_get_str(cfg, "upload.content.guid", &p) != 0) {
        ERR("xmlcfg_get_str");
        return -1;
    }
    printf("upload.content.guid = %s\n", p);
    if (xmlcfg_get_str(cfg, "upload.content.files.file.name", &p) != 0) {
        ERR("xmlcfg_get_str");
        return -1;
    }
    printf("upload.content.files.file.name = %s\n", p);

    int64_t val = 0;
    if (xmlcfg_get_int(cfg, "upload.content.priority", &val) != 0) {
        ERR("xmlcfg_get_int");
        return -1;
    }
    printf("upload.content.priority = %ld\n", val);

    size_t size = 0;
    if (xmlcfg_get_size(cfg, "upload.content.files.file.size", &size) != 0) {
        ERR("xmlcfg_get_size");
        return -1;
    }
    printf("upload.content.files.file.size = %lu\n", size);
    return 0;
}

int main(void)
{
    test1();
    return 0;
}