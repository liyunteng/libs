/*
 * test.c - test
 *
 * Date   : 2021/01/17
 */

#include "log.h"
#include "macro.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <unistd.h>

const char *module_name = "abc";

TODO(THIS IS A TEST);

void
test_simple(void)
{
    int i;
    log_init("logs", "ihi", -1, 1);

    LOGV("this is verbose");
    LOGD("this is debug");
    LOGI("this is info");
    LOGN("this is notice");
    LOGW("this is warnning");
    LOGE("this is error");
    LOGF("this is fatal");
    LOGA("this is alert");
    LOGP("this is panic");


    log_set_level(LOG_WARNING);

    LOGV("this is verbose");
    LOGD("this is debug");
    LOGI("this is info");
    LOGN("this is notice");
    LOGW("this is warnning");
    LOGE("this is error");
    LOGF("this is fatal");
    LOGA("this is alert");
    LOGP("this is panic");

    log_dump();
    log_uninit();
}

void
test_size(void)
{
    int i;
    log_init("logs", "ihi", LOG_VERBOSE, 1);

#if 0
    log_format_t *format = log_format_create("%d.%ms [%5.5V] %m%n");
#if 1
    log_output_t *output =
        log_output_create(LOG_OUTTYPE_MMAP, "logs", "ihi", 4 *1024*1024, 0,
                          4 * 1024, 1000);
#else
    log_output_t *output = log_output_create(LOG_OUTTYPE_FILE, "logs", "ihi", 4*1024*1024, 4);
#endif
    log_handler_t *handler = log_handler_create("ihi");
    log_bind(handler, -1, -1, format, output);
    log_handler_set_default(handler);
#endif

    char buf[1024 - 32] = {0};
    memset(buf, 'a', 1024 - 33);

    for (i = 0; i < 1024; i++) {
        LOGV("%s", buf);
    }
    /* LOGV("this is verbose");
     * LOGD("this is debug");
     * LOGI("this is info");
     * LOGN("this is notice");
     * LOGW("this is warnning");
     * LOGE("this is error");
     * LOGF("this is fatal");
     * LOGA("this is alert");
     * LOGP("this is panic");
     *
     *
     * log_simple_set_level(LOG_WARNING);
     *
     * LOGV("this is verbose");
     * LOGD("this is debug");
     * LOGI("this is info");
     * LOGN("this is notice");
     * LOGW("this is warnning");
     * LOGE("this is error");
     * LOGF("this is fatal");
     * LOGA("this is alert");
     * LOGP("this is panic"); */

    log_dump();
    log_uninit();
}

int
cb(const char *ident, int level, const char *msg, int msg_len, void *priv_data)
{
    printf("%s  %s  %s", ident, (char *)priv_data, msg);
    return msg_len;
}

void
test_callback(void)
{
    const char *priv = "xxxxxx";
    log_handler_t *h = log_handler_create("ihi");
    log_format_t *f  = log_format_create("%d.%ms [%5.5V] %m%n");
    log_output_t *o  = log_output_create(LOG_OUTTYPE_USER, cb, priv);
    log_rule_t *r    = log_rule_create(h, f, o, -1, -1);

    CLOGV(h, "this is a verbose");
    CLOGD(h, "this is a debug");
    CLOGI(h, "this is a info");

    log_dump();
    log_cleanup();
}

void
x(int level,  char *tag, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_vprintf(log_handler_get_default(), level, NULL, NULL, 0,  fmt, ap);
    va_end(ap);
}

void
test_vprintf(void)
{
    const char *str  = "zzzz";
    log_handler_t *h = log_handler_create("ihi");
    log_format_t *f  = log_format_create("%d.%ms [%5.5V] %F:%U:%L %m%n");
    log_output_t *o  = log_output_create(LOG_OUTTYPE_STDOUT);
    log_rule_t *r    = log_rule_create(h, f, o, -1, -1);
    log_handler_set_default(h);

    x(LOG_INFO, "tag", "abcdefg");
    x(LOG_INFO, "tag", "this is %s len: %lu", str, 10);
}

void
test_mlog(void)
{
    log_handler_t *h1      = log_handler_create("handler1");
    log_format_t *format1  = log_format_create("%d %p %c %C%V%R %F:%U:%L %m%n");
    log_output_t *fileout1 = log_output_create(LOG_OUTTYPE_FILE, "logs",
                                               "handler1", 1024 * 1024 * 4, 4);
    log_rule_t *r1         = log_rule_create(h1, format1, fileout1, -1, -1);

    log_handler_t *h2      = log_handler_create("handler2");
    log_format_t *format2  = log_format_create("%d.%ms [%V] %m%n");
    log_output_t *std_out  = log_output_create(LOG_OUTTYPE_STDOUT);
    log_output_t *fileout2 = log_output_create(LOG_OUTTYPE_FILE, "logs",
                                               "handler2", 1024 * 1024 * 4, 4);
    log_rule_t *r2         = log_rule_create(h2, format2, fileout2, -1, -1);

    CLOGV(h1, "this is a verbose");
    CLOGV(h2, "this is a verbose");

    CLOGD(h1, "this is a debug");
    CLOGD(h2, "this is a debug");

    log_rule_destroy(r2);
    CLOGI(h1, "%s", "this is a info");
    CLOGI(h2, "%s", "this is a info");
    CLOGN(h1, "this is a notice");
    CLOGN(h2, "this is a notice");
    CLOGW(h1, "this is a warning");
    CLOGW(h2, "this is a warning");
    CLOGE(h1, "this is a error");
    CLOGE(h2, "this is a error");

    log_rule_t *r3 = log_rule_create(h2, format1, fileout2, -1, -1);
    CLOGF(h1, "this is a fatal");
    CLOGF(h2, "this is a fatal");

    CLOGA(h1, "this is a alert");
    CLOGA(h2, "this is a alert");

    CLOGP(h1, "this is a emergency");
    CLOGP(h2, "this is a emergency");


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
        log_output_create(LOG_OUTTYPE_FILE, "logs", "ihi", 1024 * 1024 * 4, 4);
#else
    log_output_t *o =
        log_output_create(LOG_OUTTYPE_MMAP, "logs", "ihi", 1024 * 1024 * 4, 4,
                          4 * 1024 * 1024, 1000);
#endif

    log_rule_create(h, f, o, -1, -1);
    log_handler_set_default(h);

    pthread_create(&tid1, NULL, run, NULL);
    pthread_create(&tid2, NULL, run, NULL);
    pthread_create(&tid3, NULL, run, NULL);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);

    log_dump();
    log_cleanup();
}

void
test_mlog_benchmark()
{
    log_handler_t *h1     = log_handler_create("h1");
    log_handler_t *h2     = log_handler_create("h2");
    log_handler_t *h3     = log_handler_create("h3");
    log_format_t *format  = log_format_create("%d %p %c %V %F:%U:%L %m%n");
    log_output_t *fileout = log_output_create(LOG_OUTTYPE_FILE, "logs", "ihi",
                                              1024 * 1024 * 1000, 10, 4);

    log_rule_create(h1, format, fileout, -1, -1);
    log_rule_create(h2, format, fileout, -1, -1);
    log_rule_create(h3, format, fileout, -1, -1);

    unsigned i;
    for (i = 0; i < 1024 * 1024 * 16; i++) {
        CLOGD(h1, "this is a debug");
        CLOGI(h1, "%s", "this is a info");
        CLOGW(h1, "this is a warning");
        CLOGE(h1, "this is a error");
        CLOGF(h1, "this is a fatal");

        CLOGD(h2, "this is a debug");
        CLOGI(h2, "%s", "this is a info");
        CLOGW(h2, "this is a warning");
        CLOGE(h2, "this is a error");
        CLOGF(h2, "this is a fatal");

        CLOGD(h3, "this is a debug");
        CLOGI(h3, "%s", "this is a info");
        CLOGW(h3, "this is a warning");
        CLOGE(h3, "this is a error");
        CLOGF(h3, "this is a fatal");
    }

    log_dump();
    log_cleanup();
}

void
test_log_benchmark()
{
    /* log_format_t *format = log_format_create("%d.%ms %c:%p [%V] %m%n"); */
    log_format_t *format = log_format_create("%d.%ms %c:%p [%V] %m%n");
#if 1
    log_output_t *output =
        log_output_create(LOG_OUTTYPE_MMAP, "logs", "ihi", 1024 * 1024 * 1024,
                          4, 4 * 1024 * 1024, 1000);
#else
    log_output_t *output = log_output_create(LOG_OUTTYPE_FILE, "logs", "ihi",
                                             1024 * 1024 * 1024, 4);
#endif
    log_handler_t *handler = log_handler_create("ihi");
    log_rule_create(handler, format, output, -1, -1);
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

    log_dump();
    log_cleanup();
}

void
test_log_big_benchmark()
{
    log_format_t *format = log_format_create("%d.%ms %c:%p [%V] %m%n");

#if 1
    log_output_t *output =
        log_output_create(LOG_OUTTYPE_MMAP, "logs", "ihi", 1024 * 1024 * 1024,
                          4, 4 * 1024 * 1024, 1 * 1000);
#else
    log_output_t *output = log_output_create(LOG_OUTTYPE_FILE, "logs", "ihi",
                                             1024 * 1024 * 1024, 4);
#endif
    log_handler_t *handler = log_handler_create("ihi");
    log_rule_create(handler, format, output, -1, -1);
    log_handler_set_default(handler);

    char b[1024 * 8] = {0};
    int i;
    for (i = 0; i < sizeof(b) - 1; i++) {
        b[i] = 'b';
    }

    for (i = 0; i < 1024 * 1024; i++) {
        LOGD("%s", b);
    }

    log_dump();
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
    log_output_t *file1   = log_output_create(LOG_OUTTYPE_MMAP, "logs", "ihi",
                                            8 * 1024 * 1024, 50, 4 * 1024, 100);
    log_output_t *file2   = log_output_create(LOG_OUTTYPE_FILE, "logs", "test",
                                            8 * 1024 * 1024, 50);
    log_output_t *sout    = log_output_create(LOG_OUTTYPE_STDOUT);
    log_output_t *serr    = log_output_create(LOG_OUTTYPE_STDERR);

    log_output_t *syslog = log_output_create(
        LOG_OUTTYPE_SYSLOG, "ihi", LOG_NDELAY | LOG_NOWAIT | LOG_PID, LOG_USER);
    log_output_t *tcp =
        log_output_create(LOG_OUTTYPE_TCP, "127.0.0.1", (unsigned)12345);
    log_output_t *udp =
        log_output_create(LOG_OUTTYPE_UDP, "127.0.0.1", (unsigned)12345);
    log_handler_t *handler = log_handler_create("ihi");
    log_handler_set_default(handler);

    log_rule_create(handler, format, file1, -1, -1);
    log_rule_create(handler, format1, file2, LOG_ERROR, -1);
    log_rule_create(handler, format, sout, -1, -1);
    log_rule_create(handler, format1, serr, -1, -1);
    log_rule_create(handler, format2, syslog, LOG_DEBUG, LOG_FATAL);
    log_rule_create(handler, format1, tcp, -1, -1);
    log_rule_create(handler, format, udp, -1, -1);

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
    }

    log_dump();
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
        log_output_create(LOG_OUTTYPE_FILE, "logs", "ihi", 1024 * 1024 * 4, 4);
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
    log_rule_t *r1 = log_rule_create(handler, format, output, -1, -1);
    if (!r1) {
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
    log_dump();

    log_rule_destroy(r1);
    log_handler_destroy(handler);
    log_format_destroy(format);
    log_output_destroy(output);

    log_dump();
    log_cleanup();
}

DEPRECATED_API void
test_big_buf()
{
    char *buf   = NULL;
    size_t size = 1024 * 1024 * 7;
    int ret;
    int i;

    log_format_t *format = log_format_create("%d.%ms %c:%p [%V] %m%n");
#if 0
    log_output_t *output =
        log_output_create(LOG_OUTTYPE_MMAP, "logs", "ihi", 3 * 1024 * 1024, 5,
                          4 * 1024, 100);
#else
    log_output_t *output =
        log_output_create(LOG_OUTTYPE_FILE, "logs", "ihi", 1024 * 1024 * 3, 5);
#endif
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
    log_rule_t *r1 = log_rule_create(handler, format, output, -1, -1);
    if (!r1) {
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

    log_dump();
    log_cleanup();
}

int
main(int argc, char *argv[])
{
    test_simple();
    /* test_size(); */
    /* test_callback(); */
    /* test_vprintf(); */
    /* test_mlog(); */

    /* test_log_thread(); */
    /* test_multi_output(); */

    /* test_format(); */
    /* test_big_buf(); */

    /* test_mlog_benchmark(); */
    /* test_log_benchmark(); */
    /* test_log_big_benchmark(); */

    return 0;
}
