/*
 * log.h - log
 *
 * Date   : 2021/01/14
 */
#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DEFAULT_IDENT "ident"


// format
//%d(%F %T)  timeformat
//%E(LOGNAME) env
//%T hh:mm:ss
//%D YYYY-MM-DD
//%ms ms
//%us us
//%H hostname
//%c ident
//%V LEVEL
//%F file
//%U func
//%L line
//%m content
//%n \n
//%p pid
//%t tid
//%% %

typedef enum {
    LOG_EMERG,              /* 0 */
    LOG_ALERT,              /* 1 */
    LOG_CRIT,               /* 2 */
    LOG_FATAL = LOG_CRIT,   /* 2 */
    LOG_ERR,                /* 3 */
    LOG_ERROR = LOG_ERR,    /* 3 */
    LOG_WARN,               /* 4 */
    LOG_WARNING = LOG_WARN, /* 4 */
    LOG_NOTICE,             /* 5 */
    LOG_INFO,               /* 6 */
    LOG_DEBUG,              /* 7 */
    LOG_VERBOSE,            /* 8 */
} LOG_LEVEL_E;

enum LOG_OUTTYPE {
    LOG_OUTTYPE_STDOUT,
    LOG_OUTTYPE_STDERR,
    LOG_OUTTYPE_FILE,
    LOG_OUTTYPE_UDP,
    LOG_OUTTYPE_TCP,
    LOG_OUTTYPE_LOGCAT,
    LOG_OUTTYPE_SYSLOG,
    LOG_OUTTYPE_NONE,
};

enum LOG_OPTS {
    LOG_OPT_SET_HANDLER_BUFFERSIZEMIN,  /* loghandler *handler, size_t size */
    LOG_OPT_SET_HANDLER_BUFFERSIZEMAX,  /* loghandler *handler, size_t size */
    LOG_OPT_GET_HANDLER_BUFFERSIZEMIN,  /* loghandler *handler, size_t *size */
    LOG_OPT_GET_HANDLER_BUFFERSIZEMAX,  /* loghandler *handler, size_t *size */
    LOG_OPT_GET_HANDLER_BUFFERSIZEREAL, /* loghandler *handler, size_t *size */
    LOG_OPT_SET_HANDLER_IDENT,          /* loghandler *handler, char *ident */
    LOG_OPT_GET_HANDLER_IDENT,          /* loghandler *handler, char *ident */
};

struct _loghandler;
struct _logforamt;
struct _logoutput;
typedef struct _loghandler loghandler;
typedef struct _logformat logformat;
typedef struct _logoutput logoutput;

loghandler *loghandler_create(const char *ident);
loghandler *loghandler_get(const char *ident);
logformat *logformat_create(const char *format, int color);

// LOG_OUTTYPE_STDERR
// LOG_OUTTYPE_STDOUT
// LOG_OUTTYPE_LOGCAT
// LOG_OUTTYPE_SYSLOG need not arg
//
// LOG_OUTTYPE_FILE char *file_path
//                  char *log_name
//                  unsigned long filesize
//                  int bakupnum
//
// LOG_OUTTYPE_UDP
// LOG_OUTTYPE_TCP  char *addr
//                  int port
logoutput *logoutput_create(enum LOG_OUTTYPE type, ...);

// if level_begin == -1, level_begin will be set to LOG_VERBOSE
// if level_end == -1 , level_end will be set to LOG_EMERG
// This will print handler's log to output, use format, when loglevel between
// level_begin and level_end
int logbind(loghandler *handler, LOG_LEVEL_E level_beign, LOG_LEVEL_E level_end,
            logformat *format, logoutput *output);
int logunbind(loghandler *handler, logoutput *output);

int log_ctl(enum LOG_OPTS, ...);
void mlog(loghandler *handle, LOG_LEVEL_E level, const char *file,
          const char *function, long line, const char *format, ...);

void slog(LOG_LEVEL_E level, const char *file, const char *function, long line,
          const char *format, ...);

void log_dump();

#ifndef MLOG
#define MLOG(handle, level, format, ...)                                       \
    mlog(handle, level, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#endif

#ifndef MLOGV
#define MLOGV(handle, format, ...)                                             \
    MLOG(handle, LOG_VERBOSE, format, ##__VA_ARGS__)
#endif

#ifndef MLOGD
#define MLOGD(handle, format, ...)                                             \
    MLOG(handle, LOG_DEBUG, format, ##__VA_ARGS__)
#endif

#ifndef MLOGI
#define MLOGI(handle, format, ...) MLOG(handle, LOG_INFO, format, ##__VA_ARGS__)
#endif

#ifndef MLOGN
#define MLOGN(handle, format, ...)                                             \
    MLOG(handle, LOG_NOTICE, format, ##__VA_ARGS__)
#endif

#ifndef MLOGW
#define MLOGW(handle, format, ...)                                             \
    MLOG(handle, LOG_WARNING, format, ##__VA_ARGS__)
#endif

#ifndef MLOGE
#define MLOGE(handle, format, ...)                                             \
    MLOG(handle, LOG_ERROR, format, ##__VA_ARGS__)
#endif

#ifndef MLOGF
#define MLOGF(handle, format, ...)                                             \
    MLOG(handle, LOG_FATAL, format, ##__VA_ARGS__)
#endif

#ifndef MLOGA
#define MLOGA(handle, format, ...)                                             \
    MLOG(handle, LOG_ALERT, format, ##__VA_ARGS__)
#endif

#ifndef MLOGX
#define MLOGX(handle, format, ...)                                             \
    MLOG(handle, LOG_EMERG, format, ##__VA_ARGS__)
#endif


#ifndef LOG
#define LOG(level, format, ...)                                                \
    slog((LOG_LEVEL_E)level, __FILE__, __FUNCTION__, __LINE__, format,         \
         ##__VA_ARGS__)
#endif

#ifndef LOGV
#define LOGV(format, ...) LOG(LOG_VERBOSE, format, ##__VA_ARGS__)
#endif

#ifndef LOGD
#define LOGD(format, ...) LOG(LOG_DEBUG, format, ##__VA_ARGS__)
#endif

#ifndef LOGI
#define LOGI(format, ...) LOG(LOG_INFO, format, ##__VA_ARGS__)
#endif

#ifndef LOGN
#define LOGN(format, ...) LOG(LOG_NOTICE, format, ##__VA_ARGS__)
#endif

#ifndef LOGW
#define LOGW(format, ...) LOG(LOG_WARNING, format, ##__VA_ARGS__)
#endif

#ifndef LOGE
#define LOGE(format, ...) LOG(LOG_ERROR, format, ##__VA_ARGS__)
#endif

#ifndef LOGF
#define LOGF(format, ...) LOG(LOG_FATAL, format, ##__VA_ARGS__)
#endif

#ifndef LOGA
#define LOGA(format, ...) LOG(LOG_ALERT, format, ##__VA_ARGS__)
#endif

#ifndef LOGX
#define LOGX(format, ...) LOG(LOG_EMERG, format, ##__VA_ARGS__)
#endif

#define LOG_INIT(log_name, level)                                              \
    do {                                                                       \
        logformat *__format =                                                  \
            logformat_create("%d.%ms %c:%p:%t [%V] %m%n", 0);                  \
        logoutput *__output = logoutput_create(                                \
            LOG_OUTTYPE_FILE, ".", (log_name), 4 * 1024 * 1024, 4);            \
        loghandler *__handler = loghandler_create(DEFAULT_IDENT);              \
        if (__format && __output && __handler) {                               \
            logbind(__handler, LOG_DEBUG, -1, __format, __output);             \
        }                                                                      \
    } while (0)


#ifdef __cplusplus
}
#endif

#endif
