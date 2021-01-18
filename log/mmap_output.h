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

    uint32_t map_size;
    uint32_t msync_interval;

    struct {
        void *addr;
        uint32_t offset;      /* addr offset to file  */
        uint32_t data_offset; /* data offset to addr */
        uint32_t window_size; /* mmap size */
        uint32_t msync_time;
        uint32_t msync_offset;
    } mmap_window;

    int fd;
    uint32_t data_offset; /* data offset to file */
    uint32_t file_current_size;
} mmap_output_ctx;

log_output_t *mmap_output_create(void);
#endif
