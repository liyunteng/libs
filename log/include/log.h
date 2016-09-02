/**
 *   @file log.hpp
 *   @brief log
 *
 *   @author liyunteng <liyunteng@streamocean.com>
 *   @copyright CopyRight (C) 2015 StreamOcean
 *   @date Update time:  2016/08/30 18:26:33
 */

#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef ANDROID
#include <android/log.h>
#endif
    
#define DEFAULT_MAX_DST 16
#define DEFAULT_BUFFERSIZE 4096 * 4
#define DEFAULT_IDENT "ihi"
#define DEFAULT_FILENAME "ihi.log"
#define DEFAULT_BACKUP 4
#define DEFAULT_FILEMODE  (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define DEFAULT_FILESIZE 4*1024*1024
#define DEFAULT_LEVEL LOGLEVEL_DEBUG
#define DEFAULT_FORMAT "%D %T.%u %i:%p [%L] %F:%f(%l) %C%n"
//#define DEFAULT_FORMAT "%D %T [%L] %C%n"
//%D YYYY-MM-DD
//%T hh:mm:ss
//%u ms
//%F file
//%f func
//%l line
//%C content
//%n \n
//%p pid
//%i ident
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
#undef LOG_EMERG
#undef LOG_ALERT
#undef LOG_FATAL
#undef LOG_CRIT
#undef LOG_ERROR
#undef LOG_WARNING
#undef LOG_NOTICE
#undef LOG_INFO
#undef LOG_DEBUG

#define LOG_EMERG       LOGLEVEL_EMERG
#define LOG_ALERT       LOGLEVEL_ALERT
#define LOG_FATAL       LOGLEVEL_FATAL
#define LOG_CRIT        LOGLEVEL_CRIT
#define LOG_ERROR       LOGLEVEL_ERROR
#define LOG_WARNING     LOGLEVEL_WARNING
#define LOG_NOTICE      LOGLEVEL_NOTICE
#define LOG_INFO        LOGLEVEL_INFO
#define LOG_DEBUG       LOGLEVEL_DEBUG
    } LOGLEVEL;


    enum LOGDSTTYPE {
        LOGDSTTYPE_STDOUT,    
        LOGDSTTYPE_STDERR,
        LOGDSTTYPE_FILE,
        LOGDSTTYPE_SOCK,
        LOGDSTTYPE_LOGCAT,
        LOGDSTTYPE_SYSLOG,
        LOGDSTTYPE_NONE,
    };

    struct logdst {
        LOGLEVEL level;
        char format[128];
        enum LOGDSTTYPE type;
        union {
            struct {
                char filename[256];
                mode_t filemode;
                uint16_t backup;
                uint64_t filesize;
                FILE *fp;
            } file;
            struct {
                char addr[256];
                uint16_t port;
                int sockfd;
            } sock;
        } u;
    };

    enum LOG_OPTS {
        LOG_OPT_SET_BUFFERSIZE,         /* uint64_t */
        LOG_OPT_GET_BUFFERSIZE,         /* uint64_t */
        LOG_OPT_SET_DST,                /* struct logdst * */
        LOG_OPT_GET_DST_COUNT,             /* uint16_t */
        LOG_OPT_GET_DST,                /* struct logdst * */
        LOG_OPT_SET_IDENT,
        LOG_OPT_GET_IDENT,
        LOG_OPT_SET_USECOLOR,
    };

    struct _loghandler;
    typedef struct _loghandler loghandler;

    loghandler *mlog_init();
    int mlog_ctl(loghandler *,
            enum LOG_OPTS,
            ...);

    void mlog(loghandler *handle,
            LOGLEVEL level,
            const char *file,
            const char *function,
            long line,
            const char *format,
            ...);

    void mlog_dump(loghandler *handle);
#ifndef MLOG
#define MLOG(handle, level, format, ...) \
    mlog(handle, level,  __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__);
#endif

#ifndef DBG
#define DBG(handle, format, ...)                                        \
    MLOG(handle, LOGLEVEL_DEBUG, format, ##__VA_ARGS__)
#endif

#ifndef INFO
#define INFO(handle, format, ...)                                       \
    MLOG(handle, LOGLEVEL_INFO, format, ##__VA_ARGS__)
#endif

#ifndef NOTICE
#define NOTICE(handle, format, ...)                                     \
    MLOG(handle, LOGLEVEL_NOTICE, format, ##__VA_ARGS__)
#endif

#ifndef WARNING
#define WARNING(handle, format, ...)                                    \
    MLOG(handle, LOGLEVEL_WARNING, format, ##__VA_ARGS__)
#endif

#ifndef ERROR
#define ERROR(handle, format, ...)                                      \
    MLOG(handle, LOGLEVEL_ERROR, format, ##__VA_ARGS__)
#endif

#ifndef FATAL
#define FATAL(handle, format, ...)                                      \
    MLOG(handle, LOGLEVEL_FATAL, format, ##__VA_ARGS__)
#endif

#ifndef ALERT
#define ALERT(handle, format, ...)                                      \
    MLOG(handle, LOGLEVEL_ALERT, format, ##__VA_ARGS__)
#endif

#ifndef EMERG
#define EMERG(handle, format, ...)                                      \
    MLOG(handle, LOGLEVEL_EMERG, format, ##__VA_ARGS__)
#endif


    int log_init();
    int log_ctl(enum LOG_OPTS opt, ...);
    void slog(LOGLEVEL level,
            const char * file,
            const char * function,
            long line,
            const char *format,
            ...);
    void log_dump();

#define LOG(level, format, ...)                                         \
    slog((LOGLEVEL)level, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)

#define LOG_INIT(filename, level)                               \
    do {                                                        \
        log_init();                                            \
    } while (0);


#ifdef __cplusplus
}
#endif

#endif
