/*
 * log.h - log
 *
 * Date   : 2021/01/14
 */
#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#define DEFAULT_IDENT "default"

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
//%m message
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


#define MLOG(handle, level, fmt...)                                            \
    do {                                                                       \
        mlog(handle, level, __FILE__, __FUNCTION__, __LINE__, fmt);            \
    } while (0)
#define MLOGV(handle, fmt...) MLOG(handle, LOG_VERBOSE, fmt)
#define MLOGD(handle, fmt...) MLOG(handle, LOG_DEBUG, fmt)
#define MLOGI(handle, fmt...) MLOG(handle, LOG_INFO, fmt)
#define MLOGN(handle, fmt...) MLOG(handle, LOG_NOTICE, fmt)
#define MLOGW(handle, fmt...) MLOG(handle, LOG_WARNING, fmt)
#define MLOGE(handle, fmt...) MLOG(handle, LOG_ERROR, fmt)
#define MLOGF(handle, fmt...) MLOG(handle, LOG_FATAL, fmt)
#define MLOGA(handle, fmt...) MLOG(handle, LOG_ALERT, fmt)
#define MLOGX(handle, fmt...) MLOG(handle, LOG_EMERG, fmt)


#define LOG(level, fmt...)                                                     \
    do {                                                                       \
        slog(level, __FILE__, __FUNCTION__, __LINE__, fmt);                    \
    } while (0)
#define LOGV(fmt...) LOG(LOG_VERBOSE, fmt)
#define LOGD(fmt...) LOG(LOG_DEBUG, fmt)
#define LOGI(fmt...) LOG(LOG_INFO, fmt)
#define LOGN(fmt...) LOG(LOG_NOTICE, fmt)
#define LOGW(fmt...) LOG(LOG_WARNING, fmt)
#define LOGE(fmt...) LOG(LOG_ERROR, fmt)
#define LOGF(fmt...) LOG(LOG_FATAL, fmt)
#define LOGA(fmt...) LOG(LOG_ALERT, fmt)
#define LOGX(fmt...) LOG(LOG_EMERG, fmt)

#define LOG_INIT(log_name, level)                                              \
    do {                                                                       \
        logformat *__format = logformat_create("%d.%ms %c:%p [%V] %m%n", 0);   \
        logoutput *__output = logoutput_create(                                \
            LOG_OUTTYPE_FILE, ".", (log_name), 4 * 1024 * 1024, 4);            \
        loghandler *__handler = loghandler_create(DEFAULT_IDENT);              \
        if (__format && __output && __handler) {                               \
            logbind(__handler, level, -1, __format, __output);                 \
        }                                                                      \
    } while (0)


#ifdef __cplusplus
extern "C" {
#endif

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

// level_begin  -1 == LOG_VERBOSE
// level_en     -1 == LOG_EMERG
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

#ifdef __cplusplus
}
#endif

#endif
