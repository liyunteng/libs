/*
 * output.c - output
 *
 * Date   : 2021/01/15
 */
#include "log_priv.h"
#include "other_outputs.h"
#include <string.h>
#include <errno.h>

#include <syslog.h>
#ifdef ANDROID
#include <android/log.h>
#endif

int
stdout_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len)
{
    if (fwrite(buf, len, 1, stdout) != 1) {
        ERROR_LOG("fwrite failed(%s)\n", strerror(errno));
        return -1;
    }
    return len;
}

int
stderr_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len)
{
    if (fwrite(buf, len, 1, stderr) != 1) {
        ERROR_LOG("fwrite failed(%s)\n", strerror(errno));
        return -1;
    }
    return len;
}

int
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

int
syslog_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len)
{
    syslog(level, "%s", buf);
    return len;
}
