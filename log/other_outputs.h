/*
 * output.h - output
 *
 * Date   : 2021/01/15
 */
#ifndef OUTPUT_H
#define OUTPUT_H
#include "log_priv.h"


struct log_output *stderr_output_create(void);
struct log_output *stdout_output_create(void);
struct log_output *logcat_output_create(void);

#endif
