/*
 * log_format.h - log_format
 *
 * Date   : 2021/01/15
 */
#ifndef __LOG_FORMAT_H__
#define __LOG_FORMAT_H__
#include "list.h"
#include "log_buf.h"
#include <stdint.h>

struct log_spec;
struct log_event;
typedef int (*write_buf)(struct log_spec *s, struct log_event *e, log_buf_t *buf);
typedef int (*gen_msg)(struct log_spec *s, struct log_event *e);

struct log_spec {
    struct list_head spec_entry;
    gen_msg gen_msg;
    write_buf write_buf;

    char *str;
    int len;
    char time_fmt[32];
    char env_name[32];

    size_t min_width;
    size_t max_width;
    int left_fill_zeros;
    int left_adjust;
    char print_fmt[16];
};

struct log_event {
    char *ident;
    size_t ident_len;

    int level;

    const char *file;
    size_t file_len;
    const char *func;
    size_t func_len;
    long line;

    const char *fmt;
    va_list ap;

    time_t ts;
    struct timeval timestamp;
    char time_str[64];
    size_t time_str_len;

    pid_t pid;
    pid_t last_pid;
    char pid_str[32];
    size_t pid_str_len;

    pthread_t tid;
    char tid_str[32];
    size_t tid_str_len;


    char hostname[256];
    size_t hostname_len;

    struct log_buf *msg_buf;
    struct log_buf *pre_msg_buf;
};

struct log_spec *spec_create(char *pstart, char **pnext);

#endif
