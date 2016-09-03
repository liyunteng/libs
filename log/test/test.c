/*
 * test.c --
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/09/03 16:32:26
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
#include <unistd.h>

#if 0
void test1()
{
    const char *s = "this is a test.";
    loghandler *h = mlog_init();

    struct logdst dst;
    dst.level = LOG_INFO;
    dst.type = LOGDSTTYPE_FILE;
    char *format = "%D %T [%L] %C%n";
    strncpy(dst.format, format, strlen(format)+1);
    strncpy(dst.u.file.filename, "abc.log", 8);
    dst.u.file.filemode = DEFAULT_FILEMODE;
    dst.u.file.backup = 0;
    dst.u.file.filesize = 1024 * 1024 *10;
    int rt = mlog_ctl(h, LOG_OPT_SET_DST, 1, &dst);
    if (rt != 0)
        fprintf(stderr, "set dst failed\n");

    struct logdst sock;
    sock.level = LOG_ERROR;
    sock.type = LOGDSTTYPE_SOCK;
    strncpy(sock.format, DEFAULT_FORMAT, strlen(DEFAULT_FORMAT)+1);
    strncpy(sock.u.sock.addr, "127.0.0.1", 10);
    sock.u.sock.port = 34567;
    rt = mlog_ctl(h, LOG_OPT_SET_DST, 2, &sock);
    if (rt != 0)
        fprintf(stderr, "set dst failed\n");

    struct logdst std;
    std.level = LOG_DEBUG;
    std.type = LOGDSTTYPE_STDOUT;
    strncpy(std.format, DEFAULT_FORMAT, strlen(DEFAULT_FORMAT)+1);
    rt = mlog_ctl(h, LOG_OPT_SET_DST, 3, &std);
    if (rt != 0)
        fprintf(stderr, "set dst failed\n");

    int color = 1;
    mlog_ctl(h, LOG_OPT_SET_USECOLOR, color);


    DBG(h, "this is a debug");
    INFO(h, "%s","this is a info");
    NOTICE(h, "this is a notice");
    WARNING(h, "this is a warning");
    ERROR(h, "this is a error");
    FATAL(h, "this is a fatal");
    ALERT(h, "this is a alert");
    EMERG(h, "this is a emergency");

    mlog_dump(h);
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
    log_init();

    struct logdst dst;
    dst.level = LOG_INFO;
    dst.type = LOGDSTTYPE_FILE;
    char *format = "%D %T [%L] %C%n";
    strncpy(dst.format, format, strlen(format)+1);
    strncpy(dst.u.file.filename, "abc.log", 8);
    dst.u.file.filemode = DEFAULT_FILEMODE;
    dst.u.file.backup = 0;
    dst.u.file.filesize = 1024 * 1024 *10;
    int rt = log_ctl(LOG_OPT_SET_DST, 1, &dst);
    if (rt != 0)
        fprintf(stderr, "set dst failed\n");

    struct logdst sock;
    sock.level = LOG_ERROR;
    sock.type = LOGDSTTYPE_SOCK;
    strncpy(dst.format, DEFAULT_FORMAT, strlen(format)+1);
    strncpy(dst.u.sock.addr, "127.0.0.1", 10);
    sock.u.sock.port = 34567;
    rt = log_ctl(LOG_OPT_SET_DST, 2, &sock);
    if (rt != 0)
        fprintf(stderr, "set dst failed\n");


    struct logdst std;
    std.level = LOG_DEBUG;
    std.type = LOGDSTTYPE_STDOUT;
    strncpy(dst.format, DEFAULT_FORMAT, strlen(format)+1);
    rt = log_ctl(LOG_OPT_SET_DST, 3, &std);
    if (rt != 0)
        fprintf(stderr, "set dst failed\n");

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
    loghandler *h1 = mlog_init();
    loghandler *h2 = mlog_init();
    loghandler *h3 = mlog_init();

    unsigned i;
    for (i = 0; i < 1024 * 1024; i++) {
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
    mlog_dump(h1);
    mlog_dump(h2);
    mlog_dump(h3);
}

void test4()
{
    /* LOG_INIT("abc.log", LOG_DEBUG); */

    unsigned i;
    log_init();

    struct logdst dst;
    dst.level = LOG_DEBUG;
    dst.type = LOGDSTTYPE_FILE;
    char *format = "%D %T [%L] %C%n";
    strncpy(dst.format, format, strlen(format)+1);
    strncpy(dst.u.file.filename, "abc.log", 8);
    dst.u.file.filemode = 0644;
    dst.u.file.backup = 0;
    dst.u.file.filesize = 1024 * 1024 *10;
    int rt = log_ctl(LOG_OPT_SET_DST, 1, &dst);
    if (rt != 0)
        fprintf(stderr, "set dst failed: %d\n", rt);

    struct logdst std;
    std.level = LOG_DEBUG;
    std.type =LOGDSTTYPE_STDOUT;
    strncpy(std.format, format, strlen(format)+1);
    rt = log_ctl(LOG_OPT_SET_DST, 2, &std);
    if (rt != 0)
        fprintf(stderr, "set dst failed: %d\n", rt);

    for (i = 0; i < 1024; i++) {
        LOG(LOG_DEBUG, "this is a debug");
        LOG(LOG_NOTICE, "this is a notice");
        LOG(LOG_INFO, "this is a info");
        LOG(LOG_ERROR, "this is a error");
        LOG(LOG_FATAL, "this is a fatal");
    }
    log_dump();
}
#endif
void test5()
{

    char b[4096];
    int i;
    //log_init();
    LOG_INIT("ihi.log", LOG_ERROR);
    for (i = 0; i < 1024 * 4 - 1; i++) {
        b[i] = 'b';
    }
    b[i] = '\0';
    #if 1
    for (i = 0; i < 1024 ;i++) {
        LOG(LOG_DEBUG, "this is a debug");
        LOG(LOG_NOTICE, "this is a notice");
        LOG(LOG_INFO, "this is a info");
        LOG(LOG_ERROR, "this is a error");
        LOG(LOG_FATAL, "this is a fatal");
        //LOG(LOG_EMERG, "this is a long msg: %s", b);
    }
#endif

#if 0
    loghandler *h = loghandler_create("abc");
    logformat *f = logformat_create("%d %c %V %m%n", 0);
    logformat *vf = logformat_create("%d %p %c %V %F:%U:%L %m%n", 1);
    logoutput *fileout = logoutput_create(LOGOUTTYPE_FILE, "ihi.log", 1024*1024*4, 0600, 1);
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
int main(int argc, char *argv[])
{
    //test1();
    //test2();
    //test3();
    //test4();
    test5();
    return 0;
}
