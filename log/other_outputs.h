/*
 * output.h - output
 *
 * Date   : 2021/01/15
 */
#ifndef OUTPUT_H
#define OUTPUT_H
#include "log.h"
#include <unistd.h>


log_output_t *stderr_output_create(void);
log_output_t *stdout_output_create(void);
log_output_t *logcat_output_create(void);

#endif
