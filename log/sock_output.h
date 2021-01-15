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

#define DEFAULT_SOCKADDR "127.0.0.1"
#define DEFAULT_SOCKPORT 12345

typedef struct {
    char addr[256];
    uint16_t port;
    int sockfd;
} sock_output_ctx;

log_output_t *tcp_output_create(void);
log_output_t *udp_output_create(void);
#endif
