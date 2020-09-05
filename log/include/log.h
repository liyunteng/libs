/**
 *   @file log.hpp
 *   @brief log
 *
 *   @author liyunteng <liyunteng@streamocean.com>
 *   @copyright CopyRight (C) 2015 StreamOcean
 *   @date Update time:  2016/09/04 16:13:26
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

#ifdef ANDROID
#    include <android/log.h>
#endif

#define DEFAULT_IDENT "ihi"
#define DEFAULT_SOCKADDR "127.0.0.1"
#define DEFAULT_SOCKPORT 12345
#define DEFAULT_FILENAME "ihi.log"
#define DEFAULT_BAKUP 4
//#define DEFAULT_BAKUP         0
#define DEFAULT_FILEMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define DEFAULT_FILESIZE 10 * 1024 * 1024
#define DEFAULT_LEVEL LOGLEVEL_DEBUG
#define DEFAULT_TIME_FORMAT "%F %T"
#define DEFAULT_FORMAT "%d.%ms %c:%p:%t [%V] %F:%U(%L) %m%n"

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
    LOGLEVEL_EMERG,
    LOGLEVEL_ALERT,
    LOGLEVEL_CRIT,
    LOGLEVEL_FATAL = LOGLEVEL_CRIT,
    LOGLEVEL_ERROR,
    LOGLEVEL_WARNING,
    LOGLEVEL_NOTICE,
    LOGLEVEL_INFO,
    LOGLEVEL_DEBUG,
    LOGLEVEL_VERBOSE,
#undef LOG_EMERG
#undef LOG_ALERT
#undef LOG_FATAL
#undef LOG_CRIT
#undef LOG_ERROR
#undef LOG_WARNING
#undef LOG_NOTICE
#undef LOG_INFO
#undef LOG_DEBUG
#undef LOG_VERBOSE

#define LOG_EMERG LOGLEVEL_EMERG
#define LOG_ALERT LOGLEVEL_ALERT
#define LOG_FATAL LOGLEVEL_FATAL
#define LOG_CRIT LOGLEVEL_CRIT
#define LOG_ERROR LOGLEVEL_ERROR
#define LOG_WARNING LOGLEVEL_WARNING
#define LOG_NOTICE LOGLEVEL_NOTICE
#define LOG_INFO LOGLEVEL_INFO
#define LOG_DEBUG LOGLEVEL_DEBUG
#define LOG_VERBOSE LOGLEVEL_VERBOSE
} LOGLEVEL;

enum LOGOUTTYPE {
    LOGOUTTYPE_STDOUT,
    LOGOUTTYPE_STDERR,
    LOGOUTTYPE_FILE,
    LOGOUTTYPE_SOCK,
    LOGOUTTYPE_LOGCAT,
    LOGOUTTYPE_SYSLOG,
    LOGOUTTYPE_NONE,
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

// LOGOUTTYPE_STDERR, LOGOUTTYPE_STDOUT, LOGOUTTYPE_LOGCAT,
// LOGOUTTYPE_SYSLOG need not arg
// LOGOUTTYPE_FILE char *filename
//                 unsigned long filesize
//                 mode_t filemode
//                 int bakupnum
// LOGOUTTYPE_SOCK char *addr
//                 int port
logoutput *logoutput_create(enum LOGOUTTYPE type, ...);

// if level_begin == -1, level_begin will be set to LOGLEVEL_VERBOSE
// if level_end == -1 , level_end will be set to LOGLEVEL_EMERG
// This will print handler's log to output, use format, when loglevel between
// level_begin and level_end
int logbind(loghandler *handler, LOGLEVEL level_beign, LOGLEVEL level_end,
            logformat *format, logoutput *output);
int logunbind(loghandler *handler, logoutput *output);

int log_ctl(enum LOG_OPTS, ...);
void mlog(loghandler *handle, LOGLEVEL level, const char *file,
          const char *function, long line, const char *format, ...);

#ifndef MLOG
#    define MLOG(handle, level, format, ...)                                   \
        mlog(handle, level, __FILE__, __FUNCTION__, __LINE__, format,          \
             ##__VA_ARGS__);
#endif

#ifndef VERBOSE
#    define VERBOSE(handle, format, ...)                                       \
        MLOG(handle, LOGLEVEL_VERBOSE, format, ##__VA_ARGS__)
#endif

#ifndef DBG
#    define DBG(handle, format, ...)                                           \
        MLOG(handle, LOGLEVEL_DEBUG, format, ##__VA_ARGS__)
#endif

#ifndef INFO
#    define INFO(handle, format, ...)                                          \
        MLOG(handle, LOGLEVEL_INFO, format, ##__VA_ARGS__)
#endif

#ifndef NOTICE
#    define NOTICE(handle, format, ...)                                        \
        MLOG(handle, LOGLEVEL_NOTICE, format, ##__VA_ARGS__)
#endif

#ifndef WARNING
#    define WARNING(handle, format, ...)                                       \
        MLOG(handle, LOGLEVEL_WARNING, format, ##__VA_ARGS__)
#endif

#ifndef ERROR
#    define ERROR(handle, format, ...)                                         \
        MLOG(handle, LOGLEVEL_ERROR, format, ##__VA_ARGS__)
#endif

#ifndef FATAL
#    define FATAL(handle, format, ...)                                         \
        MLOG(handle, LOGLEVEL_FATAL, format, ##__VA_ARGS__)
#endif

#ifndef ALERT
#    define ALERT(handle, format, ...)                                         \
        MLOG(handle, LOGLEVEL_ALERT, format, ##__VA_ARGS__)
#endif

#ifndef EMERG
#    define EMERG(handle, format, ...)                                         \
        MLOG(handle, LOGLEVEL_EMERG, format, ##__VA_ARGS__)
#endif

void slog(LOGLEVEL level, const char *file, const char *function, long line,
          const char *format, ...);

#define LOG(level, format, ...)                                                \
    slog((LOGLEVEL)level, __FILE__, __FUNCTION__, __LINE__, format,            \
         ##__VA_ARGS__)

#define LOG_INIT(filename, level)                                              \
    do {                                                                       \
        logformat *__format = logformat_create(DEFAULT_FORMAT, 0);             \
        logoutput *__output =                                                  \
            logoutput_create(LOGOUTTYPE_FILE, (filename), DEFAULT_FILESIZE,    \
                             DEFAULT_FILEMODE, DEFAULT_BAKUP);                 \
        loghandler *__handler = loghandler_create(DEFAULT_IDENT);              \
        if (__format && __output && __handler) {                               \
            logbind(__handler, DEFAULT_LEVEL, -1, __format, __output);         \
        }                                                                      \
    } while (0)

void log_dump();
#ifdef __cplusplus
}
#endif

#endif
