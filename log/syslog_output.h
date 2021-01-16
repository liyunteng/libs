/*
 * syslog_output.h - syslog_output
 *
 * Date   : 2021/01/17
 */
#ifndef SYSLOG_OUTPUT_H
#define SYSLOG_OUTPUT_H
#include "log.h"

typedef struct {
    char *ident;
    int options;
    int facility;
} syslog_output_ctx;


log_output_t *syslog_output_create(void);

#endif
