/*
 * test.c --
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/08/31 10:41:20
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
#include <stdint.h>
#include <pthread.h>
#include <string.h>

#if 0
void test1()
{
    const char *s = "this is a test.";
    loghandler *h = log_init();

    unsigned flags = STDERRLOG | SYSLOG | LOGVERBOSE | LOGFILE | SOCKLOG;
    log_set_opt(h, LOG_OPT_FLAGS, &flags);
    unsigned long len = 1024;
    log_set_opt(h, LOG_OPT_FILESIZE, &len);
    const char *file = "abc.log";
    log_set_opt(h, LOG_OPT_FILENAME, (void *)file);
    const char *server = "127.0.0.1";
    log_set_opt(h, LOG_OPT_SERVERADDR, (void *)server);
    unsigned port = 34567;
    log_set_opt(h, LOG_OPT_SERVERPORT, &port);
    LOGLEVEL lv = LOG_WARNING;
    log_set_opt(h, LOG_OPT_LEVEL, &lv);
    unsigned bk = 0;
    log_set_opt(h, LOG_OPT_BACKUP, &bk);

    DBG(h, "this is a debug");
    INFO(h, "%s","this is a info");
    WARNING(h, "this is a warning");
    ERROR(h, "this is a error");
    FATAL(h, "this is a fatal");

    log_dump(h);

    slog_init();
    slog_set_opt(LOG_OPT_FLAGS, &flags);
    slog_set_opt(LOG_OPT_FILESIZE, &len);
    slog_set_opt(LOG_OPT_FILENAME, (void *)"ihi.log");
    slog_set_opt(LOG_OPT_SERVERADDR, (void *)server);
    slog_set_opt(LOG_OPT_SERVERPORT, &port);
    slog_set_opt(LOG_OPT_LEVEL, &lv);
    slog_set_opt(LOG_OPT_BACKUP, &bk);
    LOG(LOG_DEBUG, "this is a debug");
    LOG(LOG_INFO, "this is a info");
    LOG(LOG_WARNING, "this is a warning");
    LOG(LOG_ERROR, "this is a error");
    LOG(LOG_FATAL, "this is a fatal");
    log_dump();
}
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
    slog_init();
    unsigned flags =  LOGVERBOSE | LOGFILE ;
    slog_set_opt(LOG_OPT_FLAGS, &flags);
    slog_set_opt(LOG_OPT_FILENAME, (void *)"ihi.log");
    slog_set_opt(LOG_OPT_SERVERADDR, (void *)"127.0.0.1");
    unsigned port = 34567;
    slog_set_opt(LOG_OPT_SERVERPORT, &port);
    unsigned bak = 4;
    slog_set_opt(LOG_OPT_BACKUP, &bak);
    unsigned long len = 1024 * 1024;
    slog_set_opt(LOG_OPT_FILESIZE, &len);


    pthread_create(&tid1, NULL, run, NULL);
    pthread_create(&tid2, NULL, run, NULL);
    pthread_create(&tid3, NULL, run, NULL);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    log_dump();
}

void test3()
{
    loghandler *h1 = log_init();
    loghandler *h2 = log_init();
    loghandler *h3 = log_init();

    unsigned flags =  LOGFILE;
    unsigned long len = 1024 * 256;
    unsigned port = 34567;
    LOGLEVEL lv = LOG_WARNING;
    log_set_opt(h1, LOG_OPT_FILENAME, (void *)"h1.log");
    log_set_opt(h1, LOG_OPT_FLAGS, &flags);
    log_set_opt(h1, LOG_OPT_FILESIZE, &len);
    log_set_opt(h1, LOG_OPT_SERVERADDR, (void *)"127.0.0.1");
    log_set_opt(h1, LOG_OPT_SERVERPORT, &port);
    log_set_opt(h1, LOG_OPT_LEVEL, &lv);

    log_set_opt(h2, LOG_OPT_FILENAME, (void *)"h2.log");
    log_set_opt(h2, LOG_OPT_FLAGS, &flags);
    log_set_opt(h2, LOG_OPT_FILESIZE, &len);
    log_set_opt(h2, LOG_OPT_SERVERADDR, (void *)"127.0.0.1");
    log_set_opt(h2, LOG_OPT_SERVERPORT, &port);
    log_set_opt(h2, LOG_OPT_LEVEL, &lv);

    log_set_opt(h3, LOG_OPT_FILENAME, (void *)"h3.log");
    log_set_opt(h3, LOG_OPT_FLAGS, &flags);
    log_set_opt(h3, LOG_OPT_FILESIZE, &len);
    log_set_opt(h3, LOG_OPT_SERVERADDR, (void *)"127.0.0.1");
    log_set_opt(h3, LOG_OPT_SERVERPORT, &port);
    log_set_opt(h3, LOG_OPT_LEVEL, &lv);

    unsigned i;
    for (i = 0; i < 1024 * 10; i++) {
        DBG(h1, "this is a debug");
        INFO(h1, "%s","this is a info");
        WARNING(h1, "this is a warning");
        ERROR(h1, "this is a error");
        FATAL(h1, "this is a fatal");

        DBG(h2, "this is a debug");
        INFO(h2, "%s","this is a info");
        WARNING(h2, "this is a warning");
        ERROR(h2, "this is a error");
        FATAL(h2, "this is a fatal");

        DBG(h3, "this is a debug");
        INFO(h3, "%s","this is a info");
        WARNING(h3, "this is a warning");
        ERROR(h3, "this is a error");
        FATAL(h3, "this is a fatal");
    }
    log_dump(h1);
    log_dump(h2);
    log_dump(h3);
}

#endif
void test4()
{
    /* LOG_INIT("abc.log", LOG_DEBUG); */

    unsigned i;
    log_init();

    struct logdst dst;
    dst.level = LOG_ERROR;
    dst.type = LOGDSTTYPE_FILE;
    strncpy(dst.u.file.filename, "abc.log", 8);
    dst.u.file.filemode = 0777;
    dst.u.file.backup = 0;
    dst.u.file.filesize = 1024 * 1024 *10;
    int rt = log_ctl(LOG_OPT_SET_DST, 1, &dst);
    if (rt != 0)
        fprintf(stderr, "set dst failed: %d\n", rt);
    struct logdst std;
    dst.level = LOG_DEBUG;
    dst.type =LOGDSTTYPE_STDOUT;
    rt = log_ctl(LOG_OPT_SET_DST, 2, &std);
    if (rt != 0)
        fprintf(stderr, "set dst failed: %d\n", rt);

    for (i = 0; i < 1024 * 1024; i++) {
        LOG(LOG_DEBUG, "%lu debug", getpid());
        LOG(LOG_ERROR, "%lu error", getpid());
        LOG(LOG_FATAL, "%lu fatal", getpid());
    }
    log_dump();
}

void test5()
{
    loghandler *handle = NULL;
    handle = mlog_init();
    int i;
    for (i = 0; i < 100; i++)
        DBG(handle, "this is a test");
}
int main(int argc, char *argv[])
{
    /* test1(); */
    /* test2(); */
    /* test3(); */
    test4();
    //test5();
    return 0;
}
