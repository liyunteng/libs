/*
 * file_output.h - file_output
 *
 * Date   : 2021/01/15
 */
#ifndef FILE_OUTPUT_H
#define FILE_OUTPUT_H
#include "log.h"

#include <stdint.h>
#include <stdio.h>

typedef struct {
    char *file_path;
    char *log_name;
    uint16_t num_files;
    uint32_t file_size;
    uint16_t file_idx;
    uint32_t data_offset;
    FILE *fp;
} file_output_ctx;



int file_ctx_init(log_output_t *output, va_list ap);
void file_ctx_uninit(log_output_t *output);
void file_ctx_dump(log_output_t *output);
int file_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len);
#endif
