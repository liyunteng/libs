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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syslog.h>
#include <unistd.h>
#define TAG "abc"

void
test_mlog()
{
    LOG_INIT("ihi", -1);
    const char *s    = "this is a test.";
    log_handler_t *h = log_handler_create("h");

    log_format_t *format = log_format_create("%d %p %c %C%V%R %F:%U:%L %m%n");
    log_output_t *fileout =
        log_output_create(LOG_OUTTYPE_FILE, ".", "h", 1024 * 1024 * 4, 4);
    log_output_t *std_out = log_output_create(LOG_OUTTYPE_STDOUT);
    log_bind(h, LOG_VERBOSE, -1, format, fileout);
    log_bind(h, LOG_VERBOSE, -1, format, std_out);

    mlogv(h, "this is a verbose");
    mlogd(h, "this is a debug");
    log_unbind(h, format, fileout);
    mlogi(h, "%s", "this is a info");
    mlogn(h, "this is a notice");
    mlogw(h, "this is a warning");
    mloge(h, "this is a error");
    log_bind(h, LOG_VERBOSE, -1, format, fileout);
    mlogf(h, "this is a fatal");
    mloga(h, "this is a alert");
    mlogx(h, "this is a emergency");

    logv("this is a verbose");
    logd("this is a debug");
    logi("this is a info");
    logn("this is a notice");
    logw("this is a warning");
    loge("this is a error");
    logf("this is a fatal");
    loga("this is a alert");
    logx("this is a emergency");

    log_dump();
}

void *
run(void *arg)
{
    unsigned i;
    for (i = 0; i < 1024 * 10; i++) {
        logv("this is a verbose", (unsigned long)pthread_self());
        logd("this is a debug", (unsigned long)pthread_self());
        logi("this is a info", (unsigned long)pthread_self());
        logw("this is a warning", (unsigned long)pthread_self());
        loge("this is a error", (unsigned long)pthread_self());
        logf("this is a fatal", (unsigned long)pthread_self());
    }
    return (void *)0;
}

void
test_log_thread()
{
    pthread_t tid1, tid2, tid3;
    log_handler_t *h = log_handler_create("ihi");
    log_format_t *f  = log_format_create("%D.%ms %c:%10.10T [%-5.5V] %m%n");
    log_output_t *o =
        log_output_create(LOG_OUTTYPE_FILE, ".", "ihi", 1024 * 1024 * 4, 4);
    log_bind(h, -1, -1, f, o);
    log_handler_set_default(h);

    pthread_create(&tid1, NULL, run, NULL);
    pthread_create(&tid2, NULL, run, NULL);
    pthread_create(&tid3, NULL, run, NULL);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    log_dump();
}

void
test_mlog_benchmark()
{
    log_handler_t *h1     = log_handler_create("h1");
    log_handler_t *h2     = log_handler_create("h2");
    log_handler_t *h3     = log_handler_create("h3");
    log_format_t *format  = log_format_create("%d %p %c %V %F:%U:%L %m%n");
    log_output_t *fileout = log_output_create(LOG_OUTTYPE_FILE, ".", "ihi",
                                              1024 * 1024 * 100, 10, 4);

    log_bind(h1, LOG_VERBOSE, -1, format, fileout);
    log_bind(h2, LOG_VERBOSE, -1, format, fileout);
    log_bind(h3, LOG_VERBOSE, -1, format, fileout);

    unsigned i;
    for (i = 0; i < 1024 * 1024 * 16; i++) {
        mlogd(h1, "this is a debug");
        mlogi(h1, "%s", "this is a info");
        mlogw(h1, "this is a warning");
        mloge(h1, "this is a error");
        mlogf(h1, "this is a fatal");

        mlogd(h2, "this is a debug");
        mlogi(h2, "%s", "this is a info");
        mlogw(h2, "this is a warning");
        mloge(h2, "this is a error");
        mlogf(h2, "this is a fatal");

        mlogd(h3, "this is a debug");
        mlogi(h3, "%s", "this is a info");
        mlogw(h3, "this is a warning");
        mloge(h3, "this is a error");
        mlogf(h3, "this is a fatal");
    }
    log_dump();
}

void
test_log_benchmark()
{
    log_format_t *format = log_format_create("%d.%ms %c:%p [%V] %m%n");
    /* log_format_t *format = log_format_create("%m%n"); */
    log_output_t *output =
        log_output_create(LOG_OUTTYPE_FILE, ".", "ihi", 1024 * 1024 * 1024, 4);
    log_handler_t *handler = log_handler_create("ihi");
    log_bind(handler, -1, -1, format, output);
    log_handler_set_default(handler);

    unsigned i;
    for (i = 0; i < 1024 * 1024 * 16; i++) {
#if 0
        mlogv(handler, "this is a verbose");
        mlogd(handler, "this is a debug");
        mlogi(handler, "this is a info");
        mlogn(handler, "this is a notice");
        mlogw(handler, "this is a warning");
        mloge(handler, "this is a error");
        mlogf(handler, "this is a fatal");
        mloga(handler, "this is a alert");
        mlogx(handler, "this is a emerge");
#else
        logv("this is a verbose");
        logd("this is a debug");
        logi("this is a info");
        logn("this is a notice");
        logw("this is a warning");
        loge("this is a error");
        logf("this is a fatal");
        loga("this is a alert");
        logx("this is a emerge");
#endif
    }
    log_dump();
}

void
test_log_big_benchmark()
{
    log_format_t *format = log_format_create("%d.%ms %c:%p [%V] %m%n");
    log_output_t *output =
        log_output_create(LOG_OUTTYPE_FILE, ".", "ihi", 1024 * 1024 * 1024, 4);
    log_handler_t *handler = log_handler_create("ihi");
    log_bind(handler, -1, -1, format, output);
    log_handler_set_default(handler);

    char b[1024 * 8] = {0};
    int i;
    for (i = 0; i < sizeof(b) - 1; i++) {
        b[i] = 'b';
    }

    for (i = 0; i < 1024 * 1024; i++) {
        logd("%s", b);
    }

    log_dump();
}

void
test_multi_output()
{
    int i;
    char b[4096];
    log_format_t *format =
        log_format_create("%d.%ms %H %c %F:%U:%L %p:%t [%V] %m%n");
    log_format_t *format1 = log_format_create("%m%n");
    log_format_t *format2 = log_format_create("%m");
    log_output_t *file1 =
        log_output_create(LOG_OUTTYPE_FILE, ".", "ihi", 8 * 1024 * 1024, 50);
    log_output_t *file2 =
        log_output_create(LOG_OUTTYPE_FILE, ".", "test", 8 * 1024 * 1024, 50);
    log_output_t *sout    = log_output_create(LOG_OUTTYPE_STDOUT);
    log_output_t *serr    = log_output_create(LOG_OUTTYPE_STDOUT);

    log_output_t *syslog = log_output_create(LOG_OUTTYPE_SYSLOG, "ihi",
                                             LOG_NDELAY | LOG_NOWAIT | LOG_PID,
                                             LOG_USER);
    log_output_t *tcp =
        log_output_create(LOG_OUTTYPE_TCP, "127.0.0.1", (unsigned)12345);
    log_output_t *udp =
        log_output_create(LOG_OUTTYPE_UDP, "127.0.0.1", (unsigned)12345);
    log_handler_t *handler = log_handler_create("ihi");
    log_handler_set_default(handler);

    for (i = 0; i < sizeof(b) / sizeof(b[0]) - 1; i++) {
        b[i] = 'b';
    }
    b[i] = '\0';
    log_bind(handler, -1, -1, format, file1);
    log_bind(handler, LOG_ERROR, -1, format1, file2);
    log_bind(handler, -1, -1, format, sout);
    /* log_bind(handler, -1, -1, format, serr); */
    log_bind(handler, LOG_DEBUG, LOG_INFO, format2, syslog);
    log_bind(handler, -1, -1, format1, tcp);
    log_bind(handler, -1, -1, format, udp);


    for (i = 0; i < 102400; i++) {
        logv("this is a %s", "verbose");
        logd("this is a %s", "debug");
        logi("this is a info");
        logn("this is a notice");
        logw("this is a warnning");
        loge("this is a error");
        logf("this is a fatal");
        loga("this is a alert");
        logx("this is a emerge");
        // logx("this is a long msg: %s", b);
    }
    log_dump();
}

void
test_format()
{
    int ret;
    log_format_t *format = log_format_create(
        "%E(a) %E(LOGNAME) %H %d %D %ms %us %c %C[%-7.7V]%R %.10F:%U:%L %p %t:%T %% %m%n");
    log_output_t *output =
        log_output_create(LOG_OUTTYPE_FILE, ".", "ihi", 1024 * 1024 * 4, 4);
    log_handler_t *handler = log_handler_create("ihi");
    log_handler_set_default(handler);

    if (!format) {
        printf("format create failed\n");
        return;
    }

    if (!output) {
        printf("output create failed\n");
        return;
    }
    if (!handler) {
        printf("handler create failed\n");
        return;
    }
    ret = log_bind(handler, -1, -1, format, output);
    if (ret < 0) {
        printf("bind failed\n");
        return;
    }


    for (int i = 0; i < 10; i++) {
        logv("this is a %s", "verbose");
        logd("this is a %s", "debug");
        logi("this is a info");
        logn("this is a notice");
        logw("this is a warnning");
        loge("this is a error");
        logf("this is a fatal");
        loga("this is a alert");
        logx("this is a emerge");
    }
    log_dump();

    log_unbind(handler, format, output);
    log_handler_destroy(handler);
    log_format_destroy(format);
    log_output_destroy(output);

    log_dump();
}

void
test_big_buf()
{
    char *buf   = NULL;
    size_t size = 1024 * 1024 * 7;
    int ret;
    log_format_t *format = log_format_create("%d.%ms %c:%p [%V] %m%n");
    log_output_t *output =
        log_output_create(LOG_OUTTYPE_FILE, ".", "ihi", 1024 * 1024 * 3, 5);
    log_handler_t *handler = log_handler_create("ihi");
    log_handler_set_default(handler);

    if (!format) {
        printf("format create failed\n");
        return;
    }

    if (!output) {
        printf("output create failed\n");
        return;
    }
    if (!handler) {
        printf("handler create failed\n");
        return;
    }
    ret = log_bind(handler, -1, -1, format, output);
    if (ret < 0) {
        printf("bind failed\n");
        return;
    }

    buf = (char *)malloc(size);
    if (!buf) {
        printf("malloc failed\n");
        return;
    }
    for (int i = 0; i < size - 2; i++) {
        buf[i] = 'a';
    }

    logv("%s", buf);
    /* logd("%s", buf); */
    /* loge("%s", buf); */

    log_dump();
}

void
test_simple()
{
    LOG_INIT("ihi", LOG_VERBOSE);

    logv("this is a %s", "verbose");
    logd("this is a %s", "debug");
    logi("this is a info");
    logn("this is a notice");
    logw("this is a warnning");
    loge("this is a error");
    logf("this is a fatal");
    loga("this is a alert");
    logx("this is a emerge");

    log_dump();
}

int
main(int argc, char *argv[])
{
    /* test_simple(); */

    /* test_mlog(); */
    /* test_log_thread(); */
    /* test_multi_output(); */

    test_format();
    /* test_big_buf(); */

    /* test_mlog_benchmark(); */
    /* test_log_benchmark(); */
    /* test_log_big_benchmark(); */
    return 0;
}
