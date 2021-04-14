/*
 * output.c - output
 *
 * Date   : 2021/01/15
 */
#include "other_outputs.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#ifdef ANDROID
#include <android/log.h>
#endif

static void
generic_dump(struct log_output *output)
{
    if (output) {
        DUMP_LOG("type: %s\n", output->priv->type_name);

        dump_statstic(output);
    }
}

static int
stdout_emit(struct log_output *output, struct log_handler *handler)
{
    log_buf_t *buf = NULL;
    size_t len;

    (void)output;
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
        ERROR_LOG("fwrite failed: (%s)\n", strerror(errno));
        return -1;
    }
    return len;
}

static int
stderr_emit(struct log_output *output, struct log_handler *handler)
{
    log_buf_t *buf = NULL;
    size_t len;

    (void)output;
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
        ERROR_LOG("fwrite failed: (%s)\n", strerror(errno));
        return -1;
    }
    return len;
}

static int
logcat_emit(struct log_output *output, struct log_handler *handler)
{
    log_buf_t *buf = NULL;
    size_t len;

    (void)output;
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

struct log_output_priv stdout_output_priv = {
    .type      = LOG_OUTTYPE_STDOUT,
    .type_name = "stdout",
    .emit      = stdout_emit,
    .dump      = generic_dump,
};

struct log_output_priv stderr_output_priv = {
    .type      = LOG_OUTTYPE_STDERR,
    .type_name = "stderr",
    .emit      = stderr_emit,
    .dump      = generic_dump,
};

struct log_output_priv logcat_output_priv = {
    .type      = LOG_OUTTYPE_LOGCAT,
    .type_name = "logcat",
    .emit      = logcat_emit,
    .dump      = generic_dump,
};
