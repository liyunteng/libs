/*
 * sock_output.h - sock_output
 *
 * Date   : 2021/01/15
 */
#ifndef SOCK_OUTPUT_H
#define SOCK_OUTPUT_H
#include "log_priv.h"
#include <stdint.h>


typedef struct {
    char addr[256];
    uint16_t port;
    int sockfd;
} sock_output_ctx;

extern struct log_output_priv tcp_output_priv;
extern struct log_output_priv udp_output_priv;
#endif
