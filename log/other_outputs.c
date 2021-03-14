/*
 * output.c - output
 *
 * Date   : 2021/01/15
 */
#include "other_outputs.h"
#include "log.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>

#ifdef ANDROID
#include <android/log.h>
#endif


static void
dump_output(struct log_output *output)
{
    if (output) {
        printf("type: %s\n", output->type_name);

        dump_statstic(output);
    }
}

static int
stdout_emit(struct log_output *output, struct log_handler *handler)
{
    log_buf_t *buf = NULL;
    size_t len;

    if (!handler) {
        ERROR_LOG("handler is NULL\n");
        return -1;
    }
    buf = handler->event.msg_buf;
    if (!buf) {
        ERROR_LOG("msg_buf is NULL\n");
        return -1;
    }

    len = buf_len(buf);
    if (fwrite(buf->start, len, 1, stdout) != 1) {
        ERROR_LOG("fwrite failed(%s)\n", strerror(errno));
        return -1;
    }
    return len;
}

static int
stderr_emit(struct log_output *output, struct log_handler *handler)
{
    log_buf_t *buf = NULL;
    size_t len;
    if (!handler) {
        ERROR_LOG("handler is NULL\n");
        return -1;
    }
    buf = handler->event.msg_buf;
    if (!buf) {
        ERROR_LOG("msg_buf is NULL\n");
        return -1;
    }

    len = buf_len(buf);
    if (fwrite(buf->start, len, 1, stderr) != 1) {
        ERROR_LOG("fwrite failed(%s)\n", strerror(errno));
        return -1;
    }
    return len;
}

static int
logcat_emit(struct log_output *output, struct log_handler *handler)
{
    log_buf_t *buf = NULL;
    size_t len;
    int level;
    if (!handler) {
        ERROR_LOG("handler is NULL\n");
        return -1;
    }
    buf = handler->event.msg_buf;
    if (!buf) {
        ERROR_LOG("msg_buf is NULL\n");
        return -1;
    }

    len   = buf_len(buf);
    level = handler->event.level;

#ifdef ANDROID
    android_LogPriority pri;
    switch (level) {
    case LOG_VERBOSE:
        pri = ANDROID_LOG_VERBOSE;
        break;
    case LOG_DEBUG:
        pri = ANDROID_LOG_DEBUG;
        break;
    case LOG_INFO:
        pri = ANDROID_LOG_INFO;
        break;
    case LOG_WARNING:
        pri = ANDROID_LOG_WARN;
        break;
    case LOG_ERROR:
        pri = ANDROID_LOG_ERROR;
        break;
    case LOG_FATAL:
        pri = ANDROID_LOG_FATAL;
        break;
    case LOG_ALERT:
        pri = ANDROID_LOG_FATAL;
        break;
    case LOG_EMERG:
        pri = ANDROID_LOG_SILENT;
        break;
    default:
        pri = ANDROID_LOG_DEFAULT;
        break;
    }
    return __android_log_vprint(pri, handler->event.ident, hanlder->event.fmt,
                                handler->event.ap);
#else
    return -1;
#endif
}

struct log_output *
stderr_output_create(void)
{
    struct log_output *output = NULL;

    output = (struct log_output *)calloc(1, sizeof(struct log_output));
    if (!output) {
        ERROR_LOG("calloc failed(%s)\n", strerror(errno));
        return NULL;
    }

    output->type      = LOG_OUTTYPE_STDERR;
    output->type_name = "stderr";
    output->dump      = dump_output;
    output->emit      = stderr_emit;

    return output;
}

struct log_output *
stdout_output_create(void)
{
    struct log_output *output = NULL;

    output = (struct log_output *)calloc(1, sizeof(struct log_output));
    if (!output) {
        ERROR_LOG("calloc failed(%s)\n", strerror(errno));
        return NULL;
    }

    output->type      = LOG_OUTTYPE_STDOUT;
    output->type_name = "stdout";
    output->dump      = dump_output;
    output->emit      = stdout_emit;

    return output;
}

struct log_output *
logcat_output_create(void)
{
    struct log_output *output = NULL;

    output = (struct log_output *)calloc(1, sizeof(struct log_output));
    if (!output) {
        ERROR_LOG("calloc failed(%s)\n", strerror(errno));
        return NULL;
    }

    output->type      = LOG_OUTTYPE_LOGCAT;
    output->type_name = "logcat";
    output->dump      = dump_output;
    output->emit      = logcat_emit;

    return output;
}
