/*
 * log.h - log
 *
 * Date   : 2021/01/13
 */
#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <stdlib.h>

#define LOG_LEVEL_MAX 7
#define LOG_LEVEL_MIN 0


typedef struct {
    char *log_name;
    char *file_path;
    uint16_t num_files;
    uint32_t file_size;
    size_t file_idx;

    struct {
        int         fd;
        void        *addr;
        off_t       offset;
        size_t      len;
        off_t       data_offset;
        struct timespec msync_time;
        off_t msync_offset;
    } mmap_window;

    uint8_t level;
    uint8_t default_level;
    void *opaque;
} log_ctx_t;
typedef log_ctx_t *log_ctx_p;

#define LOG_PATH_DEFAULT "./"
#define LOG_APP_NAME_DEFAULT "test"

log_ctx_p log_init(const char *log_name, const char *file_path,
                   const uint8_t files, const uint16_t file_size);
int log_init_ex(const char *log_name, const char *file_path,
                const uint16_t files, const uint32_t file_size,
                log_ctx_p *out_ctx);
void log_destroy(log_ctx_p ctx);
int log_print(log_ctx_p ctx, const char *format, ...);

#endif
