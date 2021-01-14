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
#include <stdint.h>
#include <string.h>
#include <unistd.h>

void
test1()
{
    const char *s = "this is a test.";
    loghandler *h = loghandler_create("h");

    logformat *format = logformat_create("%d %p %c %V %F:%U:%L %m%n", 1);
    logoutput *fileout =
        logoutput_create(LOG_OUTTYPE_FILE, "ihi", 1024 * 1024 * 4, 4);
    logoutput *std_out = logoutput_create(LOG_OUTTYPE_STDOUT);
    logbind(h, LOG_DEBUG, -1, format, fileout);
    logbind(h, LOG_DEBUG, -1, format, std_out);

    MLOGD(h, "this is a debug");
    logunbind(h, fileout);
    MLOGI(h, "%s", "this is a info");
    logbind(h, LOG_DEBUG, -1, format, fileout);
    MLOGN(h, "this is a notice");
    MLOGW(h, "this is a warning");
    MLOGE(h, "this is a error");
    MLOGF(h, "this is a fatal");
    MLOGA(h, "this is a alert");
    MLOGX(h, "this is a emergency");

    log_dump();
}

#if 0
void *run(void *arg)
{
    unsigned i;
    for (i = 0; i < 1024 * 10; i++) {
        LOG(LOG_DEBUG, "%lu this is a debug", (unsigned long)pthread_self());
        LOG(LOG_INFO, "%lu this is a info", (unsigned long)pthread_self());
        LOG(LOG_WARNING, "%lu this is a warning", (unsigned long)pthread_self());
        LOG(LOG_ERROR, "%lu this is a error", (unsigned long)pthread_self());
        LOG(LOG_FATAL, "%lu this is a fatal", (unsigned long)pthread_self());
    }
}

void test2()
{
    pthread_t tid1, tid2, tid3;

    LOG_INIT("ihi", LOG_DEBUG);
    pthread_create(&tid1, NULL, run, NULL);
    pthread_create(&tid2, NULL, run, NULL);
    pthread_create(&tid3, NULL, run, NULL);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    log_dump();
}
#endif

void
test3()
{
    loghandler *h1     = loghandler_create("h1");
    loghandler *h2     = loghandler_create("h2");
    loghandler *h3     = loghandler_create("h3");
    logformat *format  = logformat_create("%d %p %c %V %F:%U:%L %m%n", 1);
    logoutput *fileout = logoutput_create(LOG_OUTTYPE_FILE, ".", "ihi",
                                          1024 * 1024 * 100, 0600, 4);

    logbind(h1, LOG_DEBUG, -1, format, fileout);
    logbind(h2, LOG_DEBUG, -1, format, fileout);
    logbind(h3, LOG_DEBUG, -1, format, fileout);

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
    LOG_INIT("abc", LOG_DEBUG);

    unsigned i;
    for (i = 0; i < 1024 * 1024 * 1024; i++) {
        LOG(LOG_DEBUG, "this is a debug");
        LOG(LOG_NOTICE, "this is a notice");
        LOG(LOG_INFO, "this is a info");
        LOG(LOG_ERROR, "this is a error");
        LOG(LOG_FATAL, "this is a fatal");
    }
    log_dump();
}

void
test5()
{

    char b[4096];
    int i;
    LOG_INIT("ihi", LOG_ERROR);
    for (i = 0; i < 1024 * 4 - 1; i++) {
        b[i] = 'b';
    }
    b[i] = '\0';
#if 1
    for (i = 0; i < 1024; i++) {
        LOG(LOG_DEBUG, "this is a debug");
        LOG(LOG_NOTICE, "this is a notice");
        LOG(LOG_INFO, "this is a info");
        LOG(LOG_ERROR, "this is a error");
        LOG(LOG_FATAL, "this is a fatal");
        LOG(LOG_EMERG, "this is a long msg: %s", b);
    }
#endif

#if 0
    loghandler *h = loghandler_create("abc");
    logformat *f = logformat_create("%d %c %V %m%n", 0);
    logformat *vf = logformat_create("%d %p %c %V %F:%U:%L %m%n", 1);
    logoutput *fileout = logoutput_create(LOGOUTTYPE_FILE, ".", "ihi", 1024*1024*4, 0600, 1);
    logoutput *out = logoutput_create(LOGOUTTYPE_STDOUT);
    //logbind(h, LOG_ERROR, -1, vf, fileout);
    logbind(h, LOG_DEBUG, -1, f, out);
    logbind(h, LOG_ERROR, -1, vf, out);
    logbind(h, LOG_ERROR, LOG_ERROR, vf, fileout);

    //loghandler *h = loghandler_get("ihi");
    for (i = 0; i < 1 ; i++) {
        DBG(h, "this is a debug");
        INFO(h, "%s","this is a info");
        NOTICE(h, "this is a notice");
        WARNING(h, "this is a warning");
        ERROR(h, "this is a error");
        FATAL(h, "this is a fatal");
        ALERT(h, "this is a alert");
        //EMERG(h, "this is a long msg: %s", b);
    }
#endif

    log_dump();
}
void
test6()
{
    LOG_INIT("ihi", LOG_DEBUG);
    LOG(LOG_DEBUG, "this is a debug");
    log_dump();
}

void
test7()
{
    int i;
    char b[4096];
    logformat *format = logformat_create("%d.%ms %H %E(USER) %c %F:%U%L %p:%t [%V] %m%n", 1);
    logoutput *output1 = logoutput_create(LOG_OUTTYPE_FILE, ".", "test", 8 * 1024 * 1024, 50);
    logoutput *output2 = logoutput_create(LOG_OUTTYPE_STDOUT);
    logoutput *output3 = logoutput_create(LOG_OUTTYPE_SYSLOG);
    logoutput *output4 = logoutput_create(LOG_OUTTYPE_TCP, "127.0.0.1", (unsigned)12345);
    loghandler *handler = loghandler_create(DEFAULT_IDENT);

    for (i = 0; i < sizeof(b)/sizeof(b[0]) - 1; i++) {
        b[i] = 'b';
    }
    b[i] = '\0';
    logbind(handler, LOG_VERBOSE, -1, format, output1);
    /* logbind(handler, LOG_VERBOSE, -1, format, output2); */
    /* logbind(handler, LOG_VERBOSE, -1, format, output3); */
    logbind(handler, LOG_VERBOSE, -1, format, output4);

    for (i = 0; i < 102400; i++) {
        LOGV("this is a verbose");
        LOGD("this is a debug");
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

int
main(int argc, char *argv[])
{
    // test1();
    // test2();
    // test3();
    // test4();
    // test5();
    // test6();
    test7();
    return 0;
}
