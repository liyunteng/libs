/*
 * user_output.h - user_output
 *
 * Date   : 2021/03/25
 */
#ifndef __USER_OUTPUT_H__
#define __USER_OUTPUT_H__

#include "log_priv.h"

typedef struct {
    log_user_callback cb;
    void *param;
} user_output_ctx;

struct log_output *user_output_create(void);

#endif /* __USER_OUTPUT_H__ */
