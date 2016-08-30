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

#define STDERRLOG       0x001
#define LOGCAT          0x002
#define SOCKLOG         0x004
#define SYSLOG          0x008
#define LOGVERBOSE      0x010
#define LOGFILE         0x020

#ifdef ANDROID
#include <android/log.h>
#endif
enum LOG_OPTS {
    LOG_OPT_FLAGS,              /* uint32_t */
    LOG_OPT_BUFFERSIZE,         /* uint64_t */
    LOG_OPT_SERVERADDR,         /* char * */
    LOG_OPT_SERVERPORT,         /* uint16_t */
    LOG_OPT_FILENAME,           /* char * */
    LOG_OPT_BACKUP,             /* uint16_t */
    LOG_OPT_FILEPERM,           /* mode_t */
    LOG_OPT_FILESIZE,           /* uint64_t */
    LOG_OPT_LEVEL,              /* LOGLEVEL */
    LOG_OPT_VERBOSEFORMAT,      /* char * */
};

typedef enum {
    LOGLEVEL_EMERG,
    LOGLEVEL_ALERT,
    LOGLEVEL_CRIT,
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
#define LOG_FATAL       LOGLEVEL_CRIT
#define LOG_CRIT        LOGLEVEL_CRIT
#define LOG_ERROR       LOGLEVEL_ERROR
#define LOG_WARNING     LOGLEVEL_WARNING
#define LOG_NOTICE      LOGLEVEL_NOTICE
#define LOG_INFO        LOGLEVEL_INFO
#define LOG_DEBUG       LOGLEVEL_DEBUG

} LOGLEVEL;

struct _loghandler;
typedef struct _loghandler loghandler;

loghandler *log_init();
int log_set_opt(loghandler *, enum LOG_OPTS, void *arg);
void mlog(loghandler *handle,
          LOGLEVEL level,
          const char *file,
          size_t filelen,
          const char *function,
          size_t functionlen,
          long line,
          const char *format,
          ...);

void vlog(loghandler *handle,
          LOGLEVEL level,
          const char *file,
          size_t filelen,
          const char *function,
          size_t functionlen,
          long line,
          const char *format,
          va_list args);
void log_dump(loghandler *handle);

#define DBG(handle, format, ...)                                        \
    mlog(handle, LOGLEVEL_DEBUG,   __FILE__, sizeof(__FILE__), __FUNCTION__, sizeof(__FUNCTION__), __LINE__, format, ##__VA_ARGS__);
#define INFO(handle, format, ...)                                       \
    mlog(handle, LOGLEVEL_INFO,    __FILE__, sizeof(__FILE__), __FUNCTION__, sizeof(__FUNCTION__), __LINE__, format, ##__VA_ARGS__);
#define WARNING(handle, format, ...)                                    \
    mlog(handle, LOGLEVEL_WARNING, __FILE__, sizeof(__FILE__), __FUNCTION__, sizeof(__FUNCTION__), __LINE__, format, ##__VA_ARGS__);
#define ERROR(handle, format, ...)                                      \
    mlog(handle, LOGLEVEL_ERROR,   __FILE__, sizeof(__FILE__), __FUNCTION__, sizeof(__FUNCTION__), __LINE__, format, ##__VA_ARGS__);
#define FATAL(handle, format, ...)                                      \
    mlog(handle, LOGLEVEL_CRIT,    __FILE__, sizeof(__FILE__), __FUNCTION__, sizeof(__FUNCTION__), __LINE__, format, ##__VA_ARGS__);
#define ALERT(handle, format, ...)                                      \
    mlog(handle, LOGLEVEL_ALERT,   __FILE__, sizeof(__FILE__), __FUNCTION__, sizeof(__FUNCTION__), __LINE__, format, ##__VA_ARGS__);
#define EMERG(handle, format, ...)                                      \
    mlog(handle, LOGLEVEL_EMERG,   __FILE__, sizeof(__FILE__), __FUNCTION__, sizeof(__FUNCTION__), __LINE__, format, ##__VA_ARGS__);


int slog_init();
int slog_set_opt(enum LOG_OPTS, void *arg);
void slog(LOGLEVEL level,
          const char * file,
          size_t filelen,
          const char * function,
          size_t functionlen,
          long line,
          const char *format,
          ...);
void slog_dump();

#define LOG(level, format, ...)                                         \
    slog((LOGLEVEL)level, __FILE__, sizeof(__FILE__), __FUNCTION__, sizeof(__FUNCTION__), __LINE__, format, ##__VA_ARGS__)

#define LOG_INIT(filename, level)                               \
    do {                                                        \
        slog_init();                                            \
        slog_set_opt(LOG_OPT_FILENAME, (void *)(filename));     \
        LOGLEVEL l = (level);                                   \
        slog_set_opt(LOG_OPT_LEVEL, &(l));                      \
    } while (0);


#ifdef __cplusplus
}
#endif


#endif
