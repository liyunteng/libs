/*
 * simple_log.h - simple_log
 *
 * Date   : 2021/04/09
 */
#ifndef __SIMPLE_LOG_H__
#define __SIMPLE_LOG_H__

#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

// just set default log_handler, simple wrapper
int simple_log_init(const char *dir, const char *filename, int level);
int simple_log_set_level(int level);
void simple_log_uninit(void);
void simple_log_enable_file(int enable);
void simple_log_enable_stdout(int enable);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* __SIMPLE_LOG_H__ */
