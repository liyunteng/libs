/*
 * sock_output.h - sock_output
 *
 * Date   : 2021/01/15
 */
#ifndef SOCK_OUTPUT_H
#define SOCK_OUTPUT_H
#include "log.h"

#include <stdint.h>
#include <stdio.h>

typedef struct {
    char addr[256];
    uint16_t port;
    int sockfd;
} sock_output_ctx;

int sock_ctx_init(log_output_t *output, va_list ap);
void sock_ctx_uninit(log_output_t *output);
void sock_ctx_dump(log_output_t *output);
int sock_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len);
#endif
