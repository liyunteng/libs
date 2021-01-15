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

#define DEFAULT_FILEPATH "."
#define DEFAULT_FILENAME "test"
#define DEFAULT_BAKUP 0
#define DEFAULT_FILESIZE 4 * 1024 * 1024

typedef struct {
    char *file_path;
    char *log_name;
    uint16_t num_files;
    uint32_t file_size;
    uint16_t file_idx;
    uint32_t data_offset;
    FILE *fp;
} file_output_ctx;

log_output_t *file_output_create(void);
#endif
