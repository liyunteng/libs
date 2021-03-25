/*
 * log.h - log
 *
 * Date   : 2021/01/14
 */
#ifndef __LOG_H__
#define __LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdint.h>

#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT 2
#define LOG_ERR 3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6
#define LOG_DEBUG 7
#define LOG_VERBOSE 8

#define LOG_ERROR LOG_ERR
#define LOG_FATAL LOG_CRIT
#define LOG_PANIC LOG_EMERG
#define LOG_WARN LOG_WARNING

enum LOG_OUTTYPE {
    LOG_OUTTYPE_STDOUT = 0x0001,
    LOG_OUTTYPE_STDERR = 0x0002,
    LOG_OUTTYPE_FILE   = 0x0004,
    LOG_OUTTYPE_MMAP   = 0x0008,
    LOG_OUTTYPE_UDP    = 0x0010,
    LOG_OUTTYPE_TCP    = 0x0020,
    LOG_OUTTYPE_LOGCAT = 0x0040,
    LOG_OUTTYPE_SYSLOG = 0x0080,
    LOG_OUTTYPE_USER   = 0x0100,
    LOG_OUTTYPE_NONE   = 0x0000,
};

typedef struct log_handler log_handler_t;
typedef struct log_format log_format_t;
typedef struct log_output log_output_t;
typedef struct log_rule log_rule_t;
typedef int (*log_user_callback)(const char *ident, int level, const char *msg,
                                 int msg_len, void *priv_data);

log_handler_t *log_handler_create(const char *ident);
void log_handler_destroy(log_handler_t *handler);
log_handler_t *log_handler_get(const char *ident);

int log_handler_set_default(log_handler_t *handler);
log_handler_t *log_handler_get_default(void);

//%d  YYYY-MM-DD HH:MM:SS
//%d(%Y/%m/%d %H:%M:%S)  YYYY/MM/DD HH:MM:SS
//%E(LOGNAME)  environment $LOGNAME
//%ms ms
//%us us
//%H hostname
//%c ident
//%V LEVEL
//%v level
//%F __FILE__
//%U __FUNC__
//%L __LINE__
//%p pid
//%t tid
//%T tid hex
//%C color
//%R color_reset
//%n '\n'
//%r '\r'
//%% '%'
//%m user message
log_format_t *log_format_create(const char *format);
void log_format_destroy(log_format_t *format);

// LOG_OUTTYPE_STDERR
// LOG_OUTTYPE_STDOUT
// LOG_OUTTYPE_LOGCAT  need no arg
//
// LOG_OUTTYPE_SYSLOG  char *ident
//                     int options
//                     int facility
//
// LOG_OUTTYPE_FILE    char *file_path
//                     char *log_name
//                     size_t file_size
//                     int bakup_num
//
// LOG_OUTTYPE_MMAP    char *file_path
//                     char *log_name
//                     size_t file_size
//                     int bakup_num
//                     size_t map_size
//                     size_t msync_interval
//
// LOG_OUTTYPE_UDP
// LOG_OUTTYPE_TCP     char *addr
//                     int port
//
// LOG_OUTTYPE_USER
//                     log_user_callback *
//                     void *priv_data
log_output_t *log_output_create(enum LOG_OUTTYPE type, ...);
void log_output_destroy(log_output_t *output);


// level_begin  -1 == LOG_VERBOSE
// level_en     -1 == LOG_EMERG
// This will print handler's log to output, use format, when loglevel between
// level_begin and level_end
log_rule_t *log_bind(log_handler_t *handler, int level_beign, int level_end,
                     log_format_t *format, log_output_t *output);
int log_unbind(log_handler_t *handler, log_rule_t *rule);
int log_set_level(log_handler_t *handler, log_rule_t *rule, int level_begin,
                  int level_end);


void mlog_printf(log_handler_t *handler, int level, const char *file,
                 const char *function, long line, const char *format, ...);
void mlog_vprintf(log_handler_t *handler, int level, const char *file,
                  const char *function, long line, const char *format,
                  va_list ap);

void log_cleanup();
void log_dump();

#define MLOG_PRINTF(handler, level, fmt...)                                    \
    do {                                                                       \
        mlog_printf(handler, level, __FILE__, __FUNCTION__, __LINE__, fmt);    \
    } while (0)
#define MLOGV(handler, fmt...) MLOG_PRINTF(handler, LOG_VERBOSE, fmt)
#define MLOGD(handler, fmt...) MLOG_PRINTF(handler, LOG_DEBUG, fmt)
#define MLOGI(handler, fmt...) MLOG_PRINTF(handler, LOG_INFO, fmt)
#define MLOGN(handler, fmt...) MLOG_PRINTF(handler, LOG_NOTICE, fmt)
#define MLOGW(handler, fmt...) MLOG_PRINTF(handler, LOG_WARNING, fmt)
#define MLOGE(handler, fmt...) MLOG_PRINTF(handler, LOG_ERROR, fmt)
#define MLOGF(handler, fmt...) MLOG_PRINTF(handler, LOG_FATAL, fmt)
#define MLOGA(handler, fmt...) MLOG_PRINTF(handler, LOG_ALERT, fmt)
#define MLOGP(handler, fmt...) MLOG_PRINTF(handler, LOG_PANIC, fmt)


#define LOGV(fmt...) MLOG_PRINTF(log_handler_get_default(), LOG_VERBOSE, fmt)
#define LOGD(fmt...) MLOG_PRINTF(log_handler_get_default(), LOG_DEBUG, fmt)
#define LOGI(fmt...) MLOG_PRINTF(log_handler_get_default(), LOG_INFO, fmt)
#define LOGN(fmt...) MLOG_PRINTF(log_handler_get_default(), LOG_NOTICE, fmt)
#define LOGW(fmt...) MLOG_PRINTF(log_handler_get_default(), LOG_WARNING, fmt)
#define LOGE(fmt...) MLOG_PRINTF(log_handler_get_default(), LOG_ERROR, fmt)
#define LOGF(fmt...) MLOG_PRINTF(log_handler_get_default(), LOG_FATAL, fmt)
#define LOGA(fmt...) MLOG_PRINTF(log_handler_get_default(), LOG_ALERT, fmt)
#define LOGP(fmt...) MLOG_PRINTF(log_handler_get_default(), LOG_PANIC, fmt)


/// simple: just for LOG*
int log_simple_init(const char *ident, int level, int enable_stdout);
int log_simple_set_level(int level);
void log_simple_uninit(void);

#ifdef __cplusplus
}
#endif

#endif
