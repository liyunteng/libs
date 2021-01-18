/*
 * mmap_output.h - mmap_output
 *
 * Date   : 2021/01/18
 */
#ifndef MMAP_OUTPUT_H
#define MMAP_OUTPUT_H
#include "log_priv.h"
#include <stdint.h>

typedef struct {
    char *file_path;
    char *log_name;
    uint16_t num_files;
    uint32_t file_size;
    uint16_t file_idx;
    int32_t data_offset; /* data offset to file */

    struct {
        void *addr;
        int32_t offset;       /* addr offset to file  */
        int32_t data_offset;  /* data offset to addr */
        uint32_t window_size; /* mmap size */
    } mmap_window;

    int fd;
} mmap_output_ctx;

log_output_t *mmap_output_create(void);
#endif
