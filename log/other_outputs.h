/*
 * output.h - output
 *
 * Date   : 2021/01/15
 */
#ifndef OUTPUT_H
#define OUTPUT_H
#include "log.h"
#include <unistd.h>

int stdout_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len);
int stderr_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len);
int logcat_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len);
int syslog_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len);

#endif
