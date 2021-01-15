/*
 * output.c - output
 *
 * Date   : 2021/01/15
 */
#include "other_outputs.h"
#include "log_priv.h"
#include <errno.h>
#include <string.h>

#include <syslog.h>
#ifdef ANDROID
#include <android/log.h>
#endif

static void
dump_output(log_output_t *output)
{
    if (output) {
        printf("type: %s\n", output->type_name);

        dump_statstic(output);
    }
}

static int
stdout_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len)
{
    if (fwrite(buf, len, 1, stdout) != 1) {
        ERROR_LOG("fwrite failed(%s)\n", strerror(errno));
        return -1;
    }
    return len;
}

static int
stderr_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len)
{
    if (fwrite(buf, len, 1, stderr) != 1) {
        ERROR_LOG("fwrite failed(%s)\n", strerror(errno));
        return -1;
    }
    return len;
}

static int
logcat_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len)
{
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
    return __android_log_vprint(l, r->indent, format, args);
#else
    return -1;
#endif
}

static int
syslog_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len)
{
    syslog(level, "%s", buf);
    return len;
}

log_output_t *
stderr_output_create(void)
{
    log_output_t *output = NULL;

    output = (log_output_t *)calloc(1, sizeof(log_output_t));
    if (!output) {
        ERROR_LOG("calloc failed(%s)\n", strerror(errno));
        return NULL;
    }

    output->type = LOG_OUTTYPE_STDERR;
    output->type_name = "stderr";
    output->dump = dump_output;
    output->emit = stderr_emit;

    return output;
}

log_output_t *
stdout_output_create(void)
{
    log_output_t *output = NULL;

    output = (log_output_t *)calloc(1, sizeof(log_output_t));
    if (!output) {
        ERROR_LOG("calloc failed(%s)\n", strerror(errno));
        return NULL;
    }

    output->type = LOG_OUTTYPE_STDOUT;
    output->type_name = "stdout";
    output->dump = dump_output;
    output->emit = stdout_emit;

    return output;
}

log_output_t *
logcat_output_create(void)
{
    log_output_t *output = NULL;

    output = (log_output_t *)calloc(1, sizeof(log_output_t));
    if (!output) {
        ERROR_LOG("calloc failed(%s)\n", strerror(errno));
        return NULL;
    }

    output->type = LOG_OUTTYPE_LOGCAT;
    output->type_name = "logcat";
    output->dump = dump_output;
    output->emit = logcat_emit;

    return output;
}

log_output_t *
syslog_output_create(void)
{
    log_output_t *output = NULL;

    output = (log_output_t *)calloc(1, sizeof(log_output_t));
    if (!output) {
        ERROR_LOG("calloc failed(%s)\n", strerror(errno));
        return NULL;
    }

    output->type = LOG_OUTTYPE_SYSLOG;
    output->type_name = "syslog";
    output->dump = dump_output;
    output->emit = syslog_emit;

    return output;
}
