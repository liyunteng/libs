/*
 * test.c - test
 *
 * Date   : 2021/01/17
 */
#include "log.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syslog.h>
#include <unistd.h>

void
test_simple()
{
    LOG_INIT("ihi", LOG_VERBOSE);

    LOGV("this is a %s", "verbose");
    LOGD("this is a %s", "debug");
    LOGI("this is a info");
    LOGN("this is a notice");
    LOGW("this is a warnning");
    LOGE("this is a error");
    LOGF("this is a fatal");
    LOGA("this is a alert");
    LOGP("this is a emerge");

    log_dump();
    log_cleanup();
}

void
test_mlog()
{
    log_handler_t *h1      = log_handler_create("handler1");
    log_format_t *format1  = log_format_create("%d %p %c %C%V%R %F:%U:%L %m%n");
    log_output_t *fileout1 = log_output_create(LOG_OUTTYPE_FILE, ".",
                                               "handler1", 1024 * 1024 * 4, 4);
    log_bind(h1, -1, -1, format1, fileout1);

    log_handler_t *h2      = log_handler_create("handler2");
    log_format_t *format2  = log_format_create("%d.%ms [%V] %m%n");
    log_output_t *std_out  = log_output_create(LOG_OUTTYPE_STDOUT);
    log_output_t *fileout2 = log_output_create(LOG_OUTTYPE_FILE, ".",
                                               "handler2", 1024 * 1024 * 4, 4);
    log_bind(h2, -1, -1, format2, fileout2);

    /* log_bind(h, LOG_VERBOSE, -1, format, std_out); */

    MLOGV(h1, "this is a verbose");
    MLOGV(h2, "this is a verbose");

    MLOGD(h1, "this is a debug");
    MLOGD(h2, "this is a debug");

    log_unbind(h2, format2, fileout2);
    MLOGI(h1, "%s", "this is a info");
    MLOGI(h2, "%s", "this is a info");
    MLOGN(h1, "this is a notice");
    MLOGN(h2, "this is a notice");
    MLOGW(h1, "this is a warning");
    MLOGW(h2, "this is a warning");
    MLOGE(h1, "this is a error");
    MLOGE(h2, "this is a error");

    log_bind(h2, -1, -1, format1, fileout2);
    MLOGF(h1, "this is a fatal");
    MLOGF(h2, "this is a fatal");

    MLOGA(h1, "this is a alert");
    MLOGA(h2, "this is a alert");

    MLOGP(h1, "this is a emergency");
    MLOGP(h2, "this is a emergency");


    log_handler_set_default(h1);
    LOGV("this is a verbose");
    LOGD("this is a debug");
    LOGI("this is a info");
    LOGN("this is a notice");
    LOGW("this is a warning");
    LOGE("this is a error");
    LOGF("this is a fatal");
    LOGA("this is a alert");
    LOGP("this is a emergency");

    log_dump();
    log_cleanup();
}

void *
run(void *arg)
{
    unsigned i;
    for (i = 0; i < 10; i++) {
        LOGV("this is a verbose");
        LOGD("this is a debug");
        LOGI("this is a info");
        LOGW("this is a warning");
        LOGE("this is a error");
        LOGF("this is a fatal");
    }
    return (void *)0;
}

void
test_log_thread()
{
    pthread_t tid1, tid2, tid3;
    log_handler_t *h = log_handler_create("ihi");
    log_format_t *f  = log_format_create("%d.%ms %c:%T [%-5.5V] %m%n");
#if 0
    log_output_t *o =
        log_output_create(LOG_OUTTYPE_FILE, ".", "ihi", 1024 * 1024 * 4, 4);
#else
    log_output_t *o =
        log_output_create(LOG_OUTTYPE_MMAP, ".", "ihi", 1024 * 1024 * 4, 4,
                          4 * 1024 * 1024, 1000);
#endif

    log_bind(h, -1, -1, f, o);
    log_handler_set_default(h);

    pthread_create(&tid1, NULL, run, NULL);
    pthread_create(&tid2, NULL, run, NULL);
    pthread_create(&tid3, NULL, run, NULL);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);

    /* log_dump(); */
    log_cleanup();
}

void
test_mlog_benchmark()
{
    log_handler_t *h1     = log_handler_create("h1");
    log_handler_t *h2     = log_handler_create("h2");
    log_handler_t *h3     = log_handler_create("h3");
    log_format_t *format  = log_format_create("%d %p %c %V %F:%U:%L %m%n");
    log_output_t *fileout = log_output_create(LOG_OUTTYPE_FILE, ".", "ihi",
                                              1024 * 1024 * 1000, 10, 4);

    log_bind(h1, -1, -1, format, fileout);
    log_bind(h2, -1, -1, format, fileout);
    log_bind(h3, -1, -1, format, fileout);

    unsigned i;
    for (i = 0; i < 1024 * 1024 * 16; i++) {
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

    /* log_dump(); */
    log_cleanup();
}

void
test_log_benchmark()
{
    /* log_format_t *format = log_format_create("%d.%ms %c:%p [%V] %m%n"); */
    log_format_t *format = log_format_create("%d.%ms %c:%p [%V] %m%n");
    log_output_t *output =
        log_output_create(LOG_OUTTYPE_MMAP, ".", "ihi", 1024 * 1024 * 1024, 4,
                          4 * 1024 * 1024, 1000);
#if 0
    log_output_t *output  = log_output_create(LOG_OUTTYPE_FILE, ".", "ihi",
                                              1024 * 1024 * 1024, 100);
#endif
    log_handler_t *handler = log_handler_create("ihi");
    log_bind(handler, -1, -1, format, output);
    log_handler_set_default(handler);

    unsigned i;
    for (i = 0; i < 1024 * 1024 * 16; i++) {
#if 0
        MLOGV(handler, "this is a verbose");
        MLOGD(handler, "this is a debug");
        MLOGI(handler, "this is a info");
        MLOGN(handler, "this is a notice");
        MLOGW(handler, "this is a warning");
        MLOGE(handler, "this is a error");
        MLOGF(handler, "this is a fatal");
        MLOGA(handler, "this is a alert");
        MLOGP(handler, "this is a emerge");
#else
        LOGV("this is a verbose");
        LOGD("this is a debug");
        LOGI("this is a info");
        LOGN("this is a notice");
        LOGW("this is a warning");
        LOGE("this is a error");
        LOGF("this is a fatal");
        LOGA("this is a alert");
        LOGP("this is a emerge");
#endif
        /* usleep(10*1000); */
    }
    LOGP("this end");

    /* log_dump(); */
    log_cleanup();
}

void
test_log_big_benchmark()
{
    log_format_t *format = log_format_create("%d.%ms %c:%p [%V] %m%n");

    log_output_t *output =
        log_output_create(LOG_OUTTYPE_MMAP, ".", "ihi", 1024 * 1024 * 1024, 4,
                          4 * 1024 * 1024, 1 * 1000);
#if 0
    log_output_t *output =
        log_output_create(LOG_OUTTYPE_FILE, ".", "ihi", 1024 * 1024 * 1024, 4);
#endif
    log_handler_t *handler = log_handler_create("ihi");
    log_bind(handler, -1, -1, format, output);
    log_handler_set_default(handler);

    char b[1024 * 8] = {0};
    int i;
    for (i = 0; i < sizeof(b) - 1; i++) {
        b[i] = 'b';
    }

    for (i = 0; i < 1024 * 1024; i++) {
        LOGD("%s", b);
    }

    /* log_dump(); */
    log_cleanup();
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
    log_output_t *sout = log_output_create(LOG_OUTTYPE_STDOUT);
    log_output_t *serr = log_output_create(LOG_OUTTYPE_STDOUT);

    log_output_t *syslog = log_output_create(
        LOG_OUTTYPE_SYSLOG, "ihi", LOG_NDELAY | LOG_NOWAIT | LOG_PID, LOG_USER);
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
    /* log_bind(handler, -1, -1, format, sout); */
    /* log_bind(handler, -1, -1, format, serr); */
    log_bind(handler, LOG_DEBUG, LOG_FATAL, format2, syslog);
    log_bind(handler, -1, -1, format1, tcp);
    log_bind(handler, -1, -1, format, udp);


    for (i = 0; i < 1; i++) {
        LOGV("this is a %s", "verbose");
        LOGD("this is a %s", "debug");
        LOGI("this is a info");
        LOGN("this is a notice");
        LOGW("this is a warnning");
        LOGE("this is a error");
        LOGF("this is a fatal");
        LOGA("this is a alert");
        LOGP("this is a emerge");
        // LOGP("this is a long msg: %s", b);
    }

    /* log_dump(); */
    log_cleanup();
}

void
test_format()
{
    int ret;
    int i;

    log_format_t *format = log_format_create(
        "%d(%y/%m/%d %H:%M:%S).%ms us(%us) %E(LOGNAME)@%H %c %p:tid<%t>:%T "
        "[%-5.5V]%C[%-5.5v]%R %.10F:%.5U:%L %m%n");
    log_output_t *output =
        log_output_create(LOG_OUTTYPE_FILE, ".", "ihi", 1024 * 1024 * 4, 4);
    log_handler_t *handler = log_handler_create("default");
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
    log_handler_set_default(handler);

    for (i = 0; i < 10; i++) {
        LOGV("this is a %s", "verbose");
        LOGD("this is a %s", "debug");
        LOGI("this is a info");
        LOGN("this is a notice");
        LOGW("this is a warnning");
        LOGE("this is a error");
        LOGF("this is a fatal");
        LOGA("this is a alert");
        LOGP("this is a emerge");
    }
    /* log_dump(); */

    log_unbind(handler, format, output);
    log_handler_destroy(handler);
    log_format_destroy(format);
    log_output_destroy(output);

    /* log_dump(); */
    log_cleanup();
}

void
test_big_buf()
{
    char *buf   = NULL;
    size_t size = 1024 * 1024 * 7;
    int ret;
    int i;

    log_format_t *format = log_format_create("%d.%ms %c:%p [%V] %m%n");
    log_output_t *output =
        log_output_create(LOG_OUTTYPE_FILE, ".", "ihi", 1024 * 1024 * 3, 5);
    log_handler_t *handler = log_handler_create("ihi");
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
    log_handler_set_default(handler);

    buf = (char *)malloc(size);
    if (!buf) {
        printf("malloc failed\n");
        return;
    }
    for (i = 0; i < size - 2; i++) {
        buf[i] = 'a';
    }

    LOGV("%s", buf);
    /* LOGD("%s", buf); */
    /* LOGE("%s", buf); */

    /* log_dump(); */
    log_cleanup();
}


int
main(int argc, char *argv[])
{
    /* test_simple(); */
    /* test_mlog(); */

    /* test_log_thread(); */
    // test_multi_output();

    /* test_format(); */
    /* test_big_buf(); */

    /* test_mlog_benchmark(); */
    test_log_benchmark();
    /* test_log_big_benchmark(); */

    return 0;
}
