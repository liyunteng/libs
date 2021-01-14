/*
 * test.c --
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/09/04 16:20:24
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "log.h"
#include <stdio.h>
#include <pthread.h>

void
test1()
{
    const char *s = "this is a test.";
    log_handler_t *h = log_handler_create("h");

    log_format_t *format = log_format_create("%d %p %c %V %F:%U:%L %m%n", 1);
    log_output_t*fileout =
        log_output_create(LOG_OUTTYPE_FILE, ".", "ihi", 1024 * 1024 * 4, 4);
    log_output_t*std_out = log_output_create(LOG_OUTTYPE_STDOUT);
    log_bind(h, LOG_VERBOSE, -1, format, fileout);
    log_bind(h, LOG_VERBOSE, -1, format, std_out);

    MLOGV(h, "this is a verbose");
    MLOGD(h, "this is a debug");
    log_unbind(h, fileout);
    MLOGI(h, "%s", "this is a info");
    MLOGN(h, "this is a notice");
    MLOGW(h, "this is a warning");
    MLOGE(h, "this is a error");
    log_bind(h, LOG_VERBOSE, -1, format, fileout);
    MLOGF(h, "this is a fatal");
    MLOGA(h, "this is a alert");
    MLOGX(h, "this is a emergency");

    log_dump();
}

void *run(void *arg)
{
    unsigned i;
    for (i = 0; i < 1024 * 10; i++) {
        LOGV("%lu this is a verbose", (unsigned long)pthread_self());
        LOGD("%lu this is a debug", (unsigned long)pthread_self());
        LOGI("%lu this is a info", (unsigned long)pthread_self());
        LOGW("%lu this is a warning", (unsigned long)pthread_self());
        LOGE("%lu this is a error", (unsigned long)pthread_self());
        LOGF("%lu this is a fatal", (unsigned long)pthread_self());
    }
    return (void *)0;
}

void test2()
{
    pthread_t tid1, tid2, tid3;

    LOG_INIT("ihi", LOG_VERBOSE);
    pthread_create(&tid1, NULL, run, NULL);
    pthread_create(&tid2, NULL, run, NULL);
    pthread_create(&tid3, NULL, run, NULL);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    log_dump();
}

void
test3()
{
    log_handler_t *h1     = log_handler_create("h1");
    log_handler_t *h2     = log_handler_create("h2");
    log_handler_t *h3     = log_handler_create("h3");
    log_format_t *format  = log_format_create("%d %p %c %V %F:%U:%L %m%n", 1);
    log_output_t*fileout = log_output_create(LOG_OUTTYPE_FILE, ".", "ihi",
                                          1024 * 1024 * 100, 0600, 4);

    log_bind(h1, LOG_VERBOSE, -1, format, fileout);
    log_bind(h2, LOG_VERBOSE, -1, format, fileout);
    log_bind(h3, LOG_VERBOSE, -1, format, fileout);

    unsigned i;
    for (i = 0; i < 1024 * 1024; i++) {
        MLOGD(h1, "this is a debug");
        MLOGI(h1, "%s", "this is a info");
        MLOGW(h1, "this is a warning");
        MLOGE(h1, "this is a error");
        MLOGF(h1, "this is a fatal");

        MLOGD(h2, "this is a debug");
        MLOGI(h2, "%s", "this is a info");
        MLOGW(h2, "this is a warning");
        MLOGE(h2, "this is a error");
        MLOGF(h2, "this is a fatal");

        MLOGD(h3, "this is a debug");
        MLOGI(h3, "%s", "this is a info");
        MLOGW(h3, "this is a warning");
        MLOGE(h3, "this is a error");
        MLOGF(h3, "this is a fatal");
    }
    log_dump();
}

void
test4()
{
    log_format_t *format = log_format_create("%d.%ms %c:%p [%V] %m%n", 0);
    log_output_t*output = log_output_create(LOG_OUTTYPE_FILE, ".", "ihi", 1024 * 1024 * 1024, 4);
    log_handler_t *handler = log_handler_create(DEFAULT_IDENT);
    log_bind(handler, -1, -1, format, output);

    unsigned i;
    for (i = 0; i < 1024 * 1024; i++) {
        LOGV("this is a verbose");
        LOGD("this is a debug");
        LOGI("this is a info");
        LOGN("this is a notice");
        LOGW("this is a warning");
        LOGE("this is a error");
        LOGF("this is a fatal");
        LOGA("this is a alert");
        LOGX("this is a emerge");
    }
    log_dump();
}

void
test5()
{
    log_format_t *format = log_format_create("%d.%ms %c:%p [%V] %m%n", 0);
    log_output_t*output = log_output_create(LOG_OUTTYPE_FILE, ".", "ihi", 1024 * 1024 * 1024, 4);
    log_handler_t *handler = log_handler_create(DEFAULT_IDENT);
    log_bind(handler, -1, -1, format, output);

    char b[1024 * 8] = {0};
    int i;
    for (i = 0; i < sizeof(b) - 1; i++) {
        b[i] = 'b';
    }

    for (i = 0; i < 1024 * 1024; i++) {
        LOGD("%s", b);
    }

    log_dump();
}

void test6()
{
    int i;
    char b[4096];
    log_format_t *format = log_format_create("%d.%ms %H %E(USER) %c %F:%U%L %p:%t [%V] %m%n", 1);
    log_format_t *format1 = log_format_create("%m", 0);
    log_output_t*output1 = log_output_create(LOG_OUTTYPE_FILE, ".", "ihi", 8 * 1024 * 1024, 50);
    log_output_t*output2 = log_output_create(LOG_OUTTYPE_STDOUT);
    log_output_t*output3 = log_output_create(LOG_OUTTYPE_SYSLOG);
    log_output_t*output4 = log_output_create(LOG_OUTTYPE_TCP, "127.0.0.1", (unsigned)12345);
    log_handler_t *handler = log_handler_create(DEFAULT_IDENT);

    for (i = 0; i < sizeof(b)/sizeof(b[0]) - 1; i++) {
        b[i] = 'b';
    }
    b[i] = '\0';
    log_bind(handler, LOG_VERBOSE, -1, format, output1);
    /* log_bind(handler, LOG_VERBOSE, -1, format, output2); */
    log_bind(handler, LOG_DEBUG, LOG_INFO, format1, output3);
    log_bind(handler, LOG_VERBOSE, -1, format, output4);

    for (i = 0; i < 102400; i++) {
        LOGV("this is a %s", "verbose");
        LOGD("this is a %s", "debug");
        LOGI("this is a info");
        LOGN("this is a notice");
        LOGW("this is a warnning");
        LOGE("this is a error");
        LOGF("this is a fatal");
        LOGA("this is a alert");
        LOGX("this is emerge");
        //LOGX("this is a long msg: %s", b);
    }
    log_dump();
}

void test7()
{
    LOG_INIT("ihi", LOG_VERBOSE);

    for (int i = 0; i < 10; i++) {
        LOGV("this is a %s", "verbose");
        LOGD("this is a %s", "debug");
        LOGI("this is a info");
        LOGN("this is a notice");
        LOGW("this is a warnning");
        LOGE("this is a error");
        LOGF("this is a fatal");
        LOGA("this is a alert");
        LOGX("this is emerge");
    }
    log_dump();
}

int
main(int argc, char *argv[])
{
    /* test1(); */
    /* test2(); */
    /* test3(); */
    /* test4(); */
    /* test5(); */
    /* test6(); */
    test7();
    return 0;
}
