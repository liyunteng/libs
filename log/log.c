/**
 *   @file log.cpp
 *   @brief log
 *
 *   @author liyunteng <liyunteng@streamocean.com>
 *   @copyright CopyRight (C) 2015 StreamOcean
 *   @date Update time:  2016/08/30 18:26:31
 */

#include <pthread.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

#include <time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <syslog.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>


#include "log.h"

#define DEFAULT_LOG_MAX_INSTANCE 256

#define DEFAULT_FLAGS LOGFILE
#define DEFAULT_BUFFERSIZE 4096 * 4
#define DEFAULT_SERVERADDR "bj-w.ml.streamocean.com"
#define DEFAULT_SERVERPORT 34567
#define DEFAULT_FILENAME "ihi.log"
#define DEFAULT_BACKUP 4
#define DEFAULT_FILEPERM  (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define DEFAULT_FILESIZE 10*1024*1024
#define DEFAULT_LEVEL LOGLEVEL_DEBUG
#define DEFAULT_VERBOSEFORMAT "%Y-%m-%d %H:%M:%S"

struct  _loghandler {
    pthread_mutex_t mutex;
    uint32_t flags;

    LOGLEVEL level;

    char *bufferp;
    uint64_t buffer_size;
    char verbose_format[128];

    // int fd;
    char file_name[256];
    FILE *fp;
    uint16_t backup;
    uint64_t file_size;
    mode_t file_mode;

    char server_addr[256];
    uint16_t server_port;
    int sockfd;
    struct sockaddr_in addr;

    uint64_t success_count;
    uint64_t success_byte;
    size_t fprintf_fail_count;
    size_t makebak_count;
    uint64_t debug_count;
    uint64_t info_count;
    uint64_t warning_count;
    uint64_t error_count;
    uint64_t fatal_count;
    uint64_t alert_count;
    uint64_t emerg_count;
    uint64_t unhandle_count;
};

static loghandler *sloger = NULL;
static const char * const LOGLEVELSTR[] = {
    "EMERG",
    "ALERT",
    "FATAL",
    "ERROR",
    "WARN",
    "NOTICE",
    "INFO",
    "DEBUG",
};

static inline void _dobak(loghandler *handle)
{
    int bakcnt;
    char old_bakfile[256];
    char new_bakfile[256];
    bakcnt = handle->backup;
    while (bakcnt >= 0) {
        if (bakcnt == 0) {
            snprintf(old_bakfile, sizeof(old_bakfile), "%s", handle->file_name);
        } else {
            snprintf(old_bakfile, sizeof(old_bakfile), "%s.%d", handle->file_name, bakcnt-1);
        }
        if(!access(old_bakfile, F_OK)) {
            if (bakcnt == handle->backup) {
                unlink(old_bakfile);
                /* fprintf(stderr, "unlink %s bakcnt: %d\n", old_bakfile, bakcnt); */
                continue;
            } else {
                snprintf(new_bakfile, sizeof(new_bakfile), "%s.%d", handle->file_name, bakcnt);
                /* fprintf(stderr, "rename %s to %s bakcnt: %d\n", old_bakfile, new_bakfile, bakcnt); */
                rename(old_bakfile, new_bakfile);

            }
        }
        bakcnt--;
    }
    handle->makebak_count++;
}

static inline void checkbak(loghandler *handle, size_t len)
{
    if (handle == NULL)
        return;

    if (handle->backup <= 0)
        return;

    /* create file */
    if (access(handle->file_name, F_OK)) {
        mode_t mask_old = umask(~(handle->file_mode));
        handle->fp = fopen(handle->file_name, "a+");
        umask(mask_old);
        return;
    }

    /* bak old log file */
    if (handle->fp == NULL) {
        _dobak(handle);
        mode_t mask_old = umask(~(handle->file_mode));
        handle->fp = fopen(handle->file_name, "a+");
        umask(mask_old);
        return;
    }

    /* limit by size */
    struct stat st;
    if (stat(handle->file_name, &st) == 0) {
        if (handle->backup > 0 && (st.st_size + len  >= handle->file_size)) {
            if (handle->fp) {
                fclose(handle->fp);
            }
            _dobak(handle);
            mode_t mask_old = umask(~(handle->file_mode));
            handle->fp = fopen(handle->file_name, "a+");
            umask(mask_old);
        }
    }
    return;
}

int slog_set_opt(enum LOG_OPTS opt, void *arg)
{
    if (sloger) {
        return log_set_opt(sloger, opt, arg);
    }
    return -1;
}

int log_set_opt(loghandler *lp, enum LOG_OPTS opt, void *arg)
{
    if (lp == NULL)
        return -1;
    switch (opt) {
    case LOG_OPT_FLAGS:
        lp->flags = *(uint32_t *)arg;
        break;
    case LOG_OPT_BUFFERSIZE:
        lp->buffer_size = (*(uint64_t *)arg);
        if (lp->bufferp) {
            free(lp->bufferp);
        }
        lp->bufferp = (char *)malloc(lp->buffer_size);
        if (lp->bufferp == NULL)
            return -1;
        break;
    case LOG_OPT_SERVERADDR:
        strncpy(lp->server_addr, (char *)arg, strlen((char *)arg)+1);
        break;
    case LOG_OPT_SERVERPORT:
        lp->server_port = (*(uint16_t *)arg);
        break;
    case LOG_OPT_FILENAME:
        strncpy(lp->file_name, (char *)arg, strlen((char *)arg)+1);
        break;
    case LOG_OPT_BACKUP:
        lp->backup = *(uint16_t *)arg;
        break;
    case LOG_OPT_FILEPERM:
        lp->file_mode = *(mode_t *)arg;
        break;
    case LOG_OPT_FILESIZE:
        lp->file_size = *(uint64_t *)arg;
        break;
    case LOG_OPT_LEVEL:
        lp->level = *(LOGLEVEL *)arg;
        break;
    case LOG_OPT_VERBOSEFORMAT:
        strncpy(lp->verbose_format, (char *)arg, strlen((char *)arg)+1);
        break;
    default:
        return -1;
    }
    return 0;
}

static int log_set_default(loghandler *lp)
{
    if (lp == NULL) {
        return -1;
    }
    memset(lp, 0, sizeof(loghandler));
    pthread_mutex_init(&(lp->mutex), NULL);
    uint32_t flags = DEFAULT_FLAGS;
    log_set_opt(lp, LOG_OPT_FLAGS, &flags);
    uint64_t size = DEFAULT_BUFFERSIZE;
    int rc = log_set_opt(lp, LOG_OPT_BUFFERSIZE, &size);
    if (rc != 0)
        return rc;
    const char *server = DEFAULT_SERVERADDR;
    log_set_opt(lp, LOG_OPT_SERVERADDR, (void *)server);
    uint16_t serverport = DEFAULT_SERVERPORT;
    log_set_opt(lp, LOG_OPT_SERVERPORT, &serverport);
    const char *filename = DEFAULT_FILENAME;
    log_set_opt(lp, LOG_OPT_FILENAME, (void *)filename);
    uint16_t backup = DEFAULT_BACKUP;
    log_set_opt(lp, LOG_OPT_BACKUP, &backup);
    mode_t perm = DEFAULT_FILEPERM;
    log_set_opt(lp, LOG_OPT_FILEPERM, &perm);
    uint64_t filesize = DEFAULT_FILESIZE;
    log_set_opt(lp, LOG_OPT_FILESIZE, &filesize);
    LOGLEVEL level = DEFAULT_LEVEL;
    log_set_opt(lp, LOG_OPT_LEVEL, &level);
    const char *verbose =  DEFAULT_VERBOSEFORMAT;
    log_set_opt(lp, LOG_OPT_VERBOSEFORMAT, (void *)verbose);

    lp->sockfd = -1;
    lp->fp = NULL;
    return 0;
}

loghandler *log_init()
{
    loghandler *lp = (loghandler *)malloc(sizeof(loghandler));
    if (lp != NULL) {
        int rc = log_set_default(lp);
        if (rc == -1) {
            free(lp);
            lp = NULL;
            fprintf(stderr, "log set default failed\n");
        }
    }
    return lp;
}

void vlog(loghandler *handle, LOGLEVEL level, const char *file, size_t filelen,
          const char *function, size_t functionlen, long line, const char *format, va_list args)
{
    (void)filelen;
    (void)functionlen;
    if (handle == NULL)
        return;

    if (level > LOGLEVEL_DEBUG)
        level = LOGLEVEL_DEBUG;
    if (level < LOGLEVEL_EMERG)
        level = LOGLEVEL_EMERG;

    if (handle->level < level) {
        handle->unhandle_count++;
        return;
    }

    if (handle->bufferp == NULL) {
        handle->unhandle_count++;
        return;
    }
    pthread_mutex_lock(&handle->mutex);
    memset(handle->bufferp, 0, handle->buffer_size);
    int idx = 0;

    if (handle->flags & LOGVERBOSE) {
        char timebuf[24];
        time_t t= time(NULL);
        struct tm now;
        localtime_r(&t, &now);
        strftime(timebuf, sizeof(timebuf), handle->verbose_format, &now);
        idx = snprintf(handle->bufferp, handle->buffer_size, "%s [%-5.5s]  %s %s():%ld ",
                       timebuf, LOGLEVELSTR[level], file, function, line);
    }

    int n = vsnprintf(handle->bufferp + idx, handle->buffer_size - idx, format, args);
    if (n < 0) {
        fprintf(stderr, "vsnprintf failed\n");
        return ;
    }

    size_t len = n + idx;
    if (len >= handle->buffer_size) {
        fprintf(stderr, "msg too long, truancated.\n");
        len = handle->buffer_size - 1;
        handle->bufferp[len-1] = '\n';
        handle->bufferp[len] = '\0';
    } else {
        handle->bufferp[len] = '\n';
        len++;
        handle->bufferp[len] = '\0';
    }

    if (handle->flags & LOGFILE) {
        #if 0
        struct stat st;
        if (stat(handle->file_name, &st) == 0) {
            if (handle->backup > 0 && (st.st_size + len  >= handle->file_size)) {
                if (handle->fp) {
                    fclose(handle->fp);
                    handle->fp = NULL;
                }
                makebak(handle);
                /*
                 * close(handle->lfp->fd);
                 * handle->lfp->fd = open(handle->lfp->logfile, O_CREAT | O_APPEND | O_WRONLY, FILEPERM);
                 */
            }
        }
        #endif
        checkbak(handle, len);

        if (handle->fp == NULL) {
            mode_t mask_old = umask(~(handle->file_mode));
            handle->fp = fopen(handle->file_name, "a+");
            umask(mask_old);
        }

        /* write(handle->lfp->fd, buf, len); */
        if (handle->fp) {
            if (fprintf(handle->fp, "%s", handle->bufferp) != (int)len) {
                if (errno != 0) {
                    perror("fprintf");
                    handle->fprintf_fail_count ++;
                }
            } else {
                handle->success_count ++;
                handle->success_byte += len;
                switch(level) {
                case LOG_DEBUG:
                    handle->debug_count++;
                    break;
                case LOG_INFO:
                    handle->info_count++;
                    break;
                case LOG_WARNING:
                    handle->warning_count++;
                    break;
                case LOG_ERROR:
                    handle->error_count++;
                    break;
                case LOG_FATAL:
                    handle->fatal_count++;
                    break;
                case LOG_ALERT:
                    handle->alert_count++;
                    break;
                case LOG_EMERG:
                    handle->emerg_count++;
                    break;
                default:
                    handle->unhandle_count++;
                }
            }
            fflush(handle->fp);
            /* fclose(handle->fp); */
        }

    }

    if (handle->flags & STDERRLOG) {
        fprintf(stderr, "%s", handle->bufferp);
    }

    if (handle->flags & LOGCAT) {
#if defined ANDROID
        android_LogPriority l;
        switch (level) {
        case LOG_DEBUG:
            l = ANDROID_LOG_DEBUG;
            break;
        case LOG_INFO:
            l = ANDROID_LOG_INFO;
            break;
        case LOG_WARNING:
            l = ANDROID_LOG_WARN;
            break;
        case LOG_ERROR:
            l = ANDROID_LOG_ERROR;
            break;
        case LOG_FATAL:
            l = ANDROID_LOG_FATAL;
            break;
        case LOG_ALERT:
            l = ANDROID_LOG_FATAL;
            break;
        case LOG_EMERG:
            l = ANDROID_LOG_SILENT;
            break;
        default:
            l = ANDROID_LOG_DEFAULT;
            break;
        }
        __android_log_vprint(l, "ihi", format, args);
#endif
    }

    if (handle->flags & SYSLOG) {
        /* openlog("test", LOG_CONS | LOG_PID, 0); */
        syslog(level, "%s", handle->bufferp);
    }

    if (handle->flags & SOCKLOG) {
        if (handle->sockfd == -1) {
            struct hostent *host = NULL;
            if ((host = gethostbyname(handle->server_addr))) {
                handle->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
                if (handle->sockfd != -1) {
                    memset(&handle->addr, 0, sizeof(handle->addr));
                    handle->addr.sin_family = AF_INET;
                    handle->addr.sin_port = htons(handle->server_port);
                    handle->addr.sin_addr = *(struct in_addr *)host->h_addr_list[0];
                }
            }
        }
        socklen_t addrlen = sizeof(handle->addr);
        if (handle->sockfd != -1)
            sendto(handle->sockfd, handle->bufferp, len+1, 0, (struct sockaddr *)&handle->addr, addrlen);
        /* handle->lfp->count++; */
        /* pthread_mutex_unlock(&logmutex); */
    }

    pthread_mutex_unlock(&handle->mutex);
}

void mlog(loghandler *handle, LOGLEVEL level, const char *file, size_t filelen,
          const char *function, size_t functionlen, long line, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(handle, level, file, filelen, function, functionlen, line, format, args);
    va_end(args);
    return;
}


int slog_init()
{
    sloger = log_init();
    if (sloger)
        return 0;
    return -1;
}

void slog(LOGLEVEL level, const char * file, size_t filelen, const char * function, size_t functionlen, long line, const char *format, ...)
{
    if (sloger) {
        va_list args;
        va_start(args, format);
        vlog(sloger, level, file, filelen, function, functionlen, line, format,  args);
        va_end(args);
    }
    return ;
}

void log_dump(loghandler *handle)
{
    if (handle) {
        printf("=============================================================\n");
        printf("output:\n");
        printf("level: %s\n", LOGLEVELSTR[handle->level]);
        if (handle->flags & LOGVERBOSE) {
            printf("verbose format: %s\n", handle->verbose_format);
        }
        if (handle->flags & STDERRLOG) {
            printf("stderr\n");
        }
        if (handle->flags & LOGCAT) {
#ifdef ANDROID
            printf("logcat\n");
#endif
        }
        if (handle->flags & SOCKLOG) {
            printf("socklog : %s:%u""\n", handle->server_addr, handle->server_port);
        }
        if (handle->flags & LOGFILE) {
            printf("logfile: %s backup: %u filesize: %"PRIu64"\n", handle->file_name, handle->backup, handle->file_size);
        }

        printf("\n");
        printf("success count: %"PRIu64"\n", handle->success_count);
        printf("success byte : %"PRIu64"\n", handle->success_byte);
        printf("fprintf fail count: %lu\n", handle->fprintf_fail_count);
        printf("make backup count: %lu\n", handle->makebak_count);
        printf("debug count: %"PRIu64"\n", handle->debug_count);
        printf("info count: %"PRIu64"\n", handle->info_count);
        printf("warning count: %"PRIu64"\n", handle->warning_count);
        printf("error count: %"PRIu64"\n", handle->error_count);
        printf("fatal count: %"PRIu64"\n", handle->fatal_count);
        printf("alert count: %"PRIu64"\n", handle->alert_count);
        printf("emerg count: %"PRIu64"\n", handle->emerg_count);
        printf("unhandle count: %"PRIu64"\n", handle->unhandle_count);
        printf("\n");
    }
}

void slog_dump(void)
{
    if (sloger) {
        log_dump(sloger);
    }
}
