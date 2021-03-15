/*
 * file_output.h - file_output
 *
 * Date   : 2021/01/15
 */
#ifndef FILE_OUTPUT_H
#define FILE_OUTPUT_H
#include "log_priv.h"

typedef struct {
    char *file_path;
    char *log_name;
    uint16_t num_files;
    uint32_t file_size;
    uint16_t file_idx;
    uint32_t data_offset;
    FILE *fp;
} file_output_ctx;

struct log_output *file_output_create(void);

#endif
