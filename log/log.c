/**
 *   @file log.cpp
 *   @brief log
 *
 *   @author liyunteng <liyunteng@streamocean.com>
 *   @copyright CopyRight (C) 2015 StreamOcean
 *   @date Update time:  2016/08/30 18:26:31
 */

#ifdef __cplusplus
#ifndef __STDC_FORMAT_MARCOS
#define __STDC_FORMAT_MARCOS
#endif
#endif

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include <syslog.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#include "log.h"

#define DEFAULT_MAX_DST 16
#define DEFAULT_BUFFERSIZE 4096 * 4
#define DEFAULT_IDENT "ihi"
#define DEFAULT_FILENAME "ihi.log"
#define DEFAULT_BACKUP 4
#define DEFAULT_FILEMODE  (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define DEFAULT_FILESIZE 10*1024*1024
#define DEFAULT_LEVEL LOGLEVEL_DEBUG
#define DEFAULT_FORMAT "%D %T.%u %i:%p [%L] %F:%f(%l) %C%n"
//#define DEFAULT_FORMAT "%D %T [%L] %C%n"

struct  _loghandler {
    pthread_mutex_t mutex;

    char *bufferp;
    size_t buffer_size;

    char ident[128];

    struct logdst* dsts[DEFAULT_MAX_DST];
    uint16_t dsts_count;

    uint64_t success_count;
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

static inline void _dobak(struct logdst *dst)
{
    int bakcnt;
    char old_bakfile[256];
    char new_bakfile[256];
    bakcnt = dst->u.file.backup;
    while (bakcnt >= 0) {
        if (bakcnt == 0) {
            snprintf(old_bakfile, sizeof(old_bakfile), "%s", dst->u.file.filename);
        } else {
            snprintf(old_bakfile, sizeof(old_bakfile), "%s.%d", dst->u.file.filename, bakcnt-1);
        }
        if(!access(old_bakfile, F_OK)) {
            if (bakcnt == dst->u.file.backup) {
                unlink(old_bakfile);
                /* fprintf(stderr, "unlink %s bakcnt: %d\n", old_bakfile, bakcnt); */
                continue;
            } else {
                snprintf(new_bakfile, sizeof(new_bakfile), "%s.%d", dst->u.file.filename, bakcnt);
                /* fprintf(stderr, "rename %s to %s bakcnt: %d\n", old_bakfile, new_bakfile, bakcnt); */
                rename(old_bakfile, new_bakfile);

            }
        }
        bakcnt--;
    }
}

static inline void checkbak(struct logdst *dst, size_t len)
{
    /* file not exist, create file */
    if (access(dst->u.file.filename, F_OK)) {
        if (dst->u.file.fp != NULL) {
            fclose(dst->u.file.fp);
            dst->u.file.fp = NULL;
        }
        mode_t mask_old = umask(~(dst->u.file.filemode));
        dst->u.file.fp = fopen(dst->u.file.filename, "a+");
        umask(mask_old);
        return;
    }

    /* dont bak, if file not open , open file */
    if (dst->u.file.backup <= 0) {
        if (dst->u.file.fp == NULL) {
            mode_t mask_old = umask(~(dst->u.file.filemode));
            dst->u.file.fp = fopen(dst->u.file.filename, "a+");
            umask(mask_old);
        }
        return;
    }

    /* file exist, but not open. */
    if (dst->u.file.fp == NULL) {
        //_dobak(dst);
        mode_t mask_old = umask(~(dst->u.file.filemode));
        dst->u.file.fp = fopen(dst->u.file.filename, "a+");
        umask(mask_old);
        return;
    }

    /* limit by size */
    struct stat st;
    if (fstat(fileno(dst->u.file.fp), &st) == 0) {
        if (dst->u.file.backup > 0 && (st.st_size + len  >= dst->u.file.filesize)) {
            if (dst->u.file.fp) {
                fclose(dst->u.file.fp);
                dst->u.file.fp = NULL;
            }
            _dobak(dst);
            mode_t mask_old = umask(~(dst->u.file.filemode));
            dst->u.file.fp = fopen(dst->u.file.filename, "a+");
            umask(mask_old);
        }
    }
    return;
}

static inline int __log_ctl(loghandler *lp, enum LOG_OPTS opt, va_list ap)
{
    switch (opt) {
        case LOG_OPT_SET_BUFFERSIZE: {
                                         lp->buffer_size = va_arg(ap, size_t);
                                         if (lp->bufferp) {
                                             free(lp->bufferp);
                                         }
                                         lp->bufferp = (char *)malloc(lp->buffer_size);
                                         if (lp->bufferp == NULL) {
                                             fprintf(stderr, "SET BUFFERSIZE failed\n");
                                             return -1;
                                         }
                                         break;
                                     }
        case LOG_OPT_GET_BUFFERSIZE: {
                                         uint64_t *size = va_arg(ap, uint64_t *);
                                         *size = lp->buffer_size;
                                         break;
                                     }
        case LOG_OPT_SET_DST: {
                                  int idx = va_arg(ap, int);
                                  struct logdst *dst = va_arg(ap, struct logdst *);
                                  if (dst == NULL || idx < 0 || idx > DEFAULT_MAX_DST)
                                      return -1;
                                  struct logdst *ndst = NULL;
                                  switch (dst->type) {
                                      case LOGDSTTYPE_STDOUT:
                                      case LOGDSTTYPE_STDERR:
                                      case LOGDSTTYPE_LOGCAT:
                                      case LOGDSTTYPE_SYSLOG:
                                          ndst = (struct logdst *) malloc(sizeof(struct logdst));
                                          if (ndst == NULL)
                                              return -1;
                                          ndst->level = dst->level;
                                          strncpy(ndst->format, dst->format, strlen(dst->format)+1);
                                          ndst->type = dst->type;
                                          break;
                                      case LOGDSTTYPE_FILE:
                                          ndst = (struct logdst *) malloc(sizeof(struct logdst));
                                          if (ndst == NULL)
                                              return -1;
                                          ndst->level = dst->level;
                                          strncpy(ndst->format, dst->format, strlen(dst->format)+1);
                                          ndst->type = dst->type;
                                          strncpy(ndst->u.file.filename, dst->u.file.filename, strlen(dst->u.file.filename)+1);
                                          ndst->u.file.fp = NULL;
                                          ndst->u.file.filemode = dst->u.file.filemode;
                                          ndst->u.file.backup = dst->u.file.backup;
                                          ndst->u.file.filesize = dst->u.file.filesize;
                                          break;
                                      case LOGDSTTYPE_SOCK:
                                          ndst = (struct logdst *) malloc(sizeof(struct logdst));
                                          if (ndst == NULL)
                                              return -1;
                                          ndst->level = dst->level;
                                          strncpy(ndst->format, dst->format, strlen(dst->format)+1);
                                          ndst->type = dst->type;
                                          strncpy(ndst->u.sock.addr, dst->u.sock.addr, strlen(dst->u.sock.addr)+1);
                                          ndst->u.sock.port = dst->u.sock.port;
                                          ndst->u.sock.sockfd = -1;
                                          break;
                                      case LOGDSTTYPE_NONE:
                                          break;
                                      default:
                                          break;
                                  }
                                  if (lp->dsts[idx] != NULL) {
                                      free(lp->dsts[idx]);
                                      lp->dsts[idx] = NULL;
                                      lp->dsts_count --;
                                  }
                                  if (ndst) {
                                      lp->dsts[idx] = ndst;
                                      lp->dsts_count ++;
                                  }
                                  break;
                              }
        case LOG_OPT_GET_DST: {
                                  int idx = va_arg(ap, int);
                                  struct logdst *dst = va_arg(ap, struct logdst *);
                                  if (idx < 0 || idx > DEFAULT_MAX_DST) {
                                      return -1;
                                      dst = NULL;
                                  }
                                  dst = lp->dsts[idx];
                                  break;
                              }
        case LOG_OPT_GET_DST_COUNT: {
                                        uint16_t *count = va_arg(ap, uint16_t *);
                                        *count = lp->dsts_count;
                                        break;
                                    }
        case LOG_OPT_SET_IDENT: {
                                    char *ident = va_arg(ap, char *);
                                    strncpy(lp->ident, ident, strlen(ident)+1);
                                    break;
                                }
        case LOG_OPT_GET_IDENT: {
                                    char *ident = va_arg(ap, char *);
                                    ident = lp->ident;
                                    break;
                                }

        default:
                                return -1;
    }

    return 0;
}

int mlog_ctl(loghandler *lp, enum LOG_OPTS opt, ...)
{
    if (lp) {
        va_list ap;
        va_start(ap, opt);
        int ret = __log_ctl(lp, opt, ap);
        va_end(ap);
        return ret;
    }
    return -1;
}

int log_ctl(enum LOG_OPTS opt, ...)
{
    if (sloger) {
        va_list ap;
        va_start(ap, opt);
        int ret = __log_ctl(sloger, opt, ap);
        va_end(ap);
        return ret;
    }
    return -1;
}

static int log_set_default(loghandler *lp)
{
    if (lp == NULL) {
        return -1;
    }
    memset(lp, 0, sizeof(loghandler));
    int i;
    for (i = 0; i < DEFAULT_MAX_DST; i++) {
        lp->dsts[i] = NULL;
    }
    pthread_mutex_init(&(lp->mutex), NULL);
    strncpy(lp->ident, DEFAULT_IDENT, strlen(DEFAULT_IDENT)+1);
    struct logdst filedst;
    filedst.type = LOGDSTTYPE_FILE;
    filedst.level = DEFAULT_LEVEL;
    strncpy(filedst.format, DEFAULT_FORMAT, strlen(DEFAULT_FORMAT)+1);
    strncpy(filedst.u.file.filename, DEFAULT_FILENAME,
            strlen(DEFAULT_FILENAME)+1);
    filedst.u.file.filemode = DEFAULT_FILEMODE;
    filedst.u.file.backup = DEFAULT_BACKUP;
    filedst.u.file.filesize = DEFAULT_FILESIZE;
    filedst.u.file.fp = NULL;
    mlog_ctl(lp, LOG_OPT_SET_DST, 0, &filedst);
#if 0
    struct logdst errdst;
    errdst.type = LOGDSTTYPE_STDERR;
    errdst.level = DEFAULT_LEVEL;
    strncpy(errdst.format, DEFAULT_FORMAT, strlen(DEFAULT_FORMAT)+1);
    mlog_ctl(lp, LOG_OPT_SET_DST, 1, &errdst);
#endif
    size_t size = DEFAULT_BUFFERSIZE;
    mlog_ctl(lp, LOG_OPT_SET_BUFFERSIZE, size);
    return 0;
}

loghandler *mlog_init()
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

static size_t parse_format(struct logdst *dst, char *buf, size_t len, 
        const char *ident, LOGLEVEL level,
        const char *file, const char *func, const long line, 
        const char *fmt, va_list ap)
{
    if (dst == NULL || buf == NULL || len <= 0) {
        return 0;
    }

    memset(buf, 0, len);
    size_t i;
    size_t idx = 0;

    time_t t = time(NULL);
    struct tm now;
    localtime_r(&t, &now);

    for (i = 0; i < strlen(dst->format); i++) {
        if (idx >= len)
            break;

        if (dst->format[i] == '%') {
            switch (dst->format[i+1]) {
                case 'D':
                    i++;
                    idx += strftime(buf+idx, len-idx,  "%F", &now);
                    break;
                case 'T':
                    i++;
                    idx += strftime(buf+idx, len-idx, "%T", &now);
                    break;
                case 'u':
                    i++;
                    struct timeval tv;
                    gettimeofday(&tv, NULL);
                    idx += snprintf(buf+idx, len-idx, "%d", (int)(tv.tv_usec/1000));
                    break;
                case 'F':
                    i++;
                    idx += snprintf(buf+idx, len-idx, "%s", file);
                    break;
                case 'f':
                    i++;
                    idx += snprintf(buf+idx, len-idx, "%s", func);
                    break;
                case 'l':
                    i++;
                    idx += snprintf(buf+idx, len-idx, "%ld", line);
                    break;
                case 'n':
                    i++;
                    idx += snprintf(buf+idx, len-idx, "%c", '\n');
                    break;
                case 'p':
                    i++;
                    idx += snprintf(buf+idx, len-idx, "%lu", getpid());
                    break;
                case 'C':
                    i++;
                    idx += vsnprintf(buf+idx, len-idx, fmt, ap);
                    break;
                case 'i':
                    i++;
                    idx += snprintf(buf+idx, len-idx, "%s", ident);
                    break;
                case 'L':
                    i++;
                    idx += snprintf(buf+idx, len-idx, "%5.5s", LOGLEVELSTR[level]);
                    break;
                case '%':
                    i++;
                    idx += snprintf(buf+idx, len-idx, "%c", dst->format[i+1]);
                    break;
                default:
                    idx += snprintf(buf+idx, len-idx, "%c", dst->format[i]);
                    break;
            }

        } else {
            idx += snprintf(buf+idx, len-idx, "%c", dst->format[i]);
        }
    }
    return idx;
}

void vlog(loghandler *handle, LOGLEVEL level, const char *file,
        const char *function, long line, const char *format, va_list args) {
    if (handle == NULL)
        return;

    if (handle->bufferp == NULL) {
        handle->unhandle_count++;
        return;
    }
    pthread_mutex_lock(&handle->mutex);

    memset(handle->bufferp, 0, handle->buffer_size);
#if 0
    int idx = 0;
    if (handle->flags & LOGFLAG_VERBOSE) {
        char timebuf[24];
        time_t t= time(NULL);
        struct tm now;
        localtime_r(&t, &now);
        strftime(timebuf, sizeof(timebuf), handle->format, &now);
        idx = snprintf(handle->bufferp, handle->buffer_size, "%s [%-5.5s]  %s %s():%ld ",
                timebuf, LOGLEVELSTR[level], file, function, line);
    }

    int n = vsnprintf(handle->bufferp + idx, handle->buffer_size - idx, format, args);
    if (n < 0) {
        fprintf(stderr, "vsnprintf failed\n");
        pthread_mutex_unlock(&handle->mutex);
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
#endif
#if 0
    int n = vsnprintf(handle->bufferp, handle->buffer_size, format, args);
    if (n < 0) {
        fprintf(stderr, "vsnprintf failed\n");
        pthread_mutex_unlock(&handle->mutex);
        return;
    }
    handle->bufferp[n] = '\n';
    handle->bufferp[n+1] = '\0';
    size_t len = n+1;
#endif

    uint16_t i;
    for (i = 0; i < DEFAULT_MAX_DST; i++) {
        if (handle->dsts[i] == NULL || handle->bufferp == NULL)
            continue;
        struct logdst *dst = handle->dsts[i];

        if (level > LOGLEVEL_DEBUG)
            level = LOGLEVEL_DEBUG;
        if (level < LOGLEVEL_EMERG)
            level = LOGLEVEL_EMERG;

        if (dst->level < level) {
            handle->unhandle_count++;
            continue;
        }
        va_list ap;
        va_copy(ap, args);
        size_t len = parse_format(dst, handle->bufferp, handle->buffer_size,
                handle->ident, level, file, function, line, format, ap);
        va_end(ap);

        switch (dst->type) {
            case LOGDSTTYPE_STDOUT:
                fprintf(stdout, "%s", handle->bufferp);
                fflush(stdout);
                break;
            case LOGDSTTYPE_STDERR:
                fprintf(stderr, "%s", handle->bufferp);
                break;
            case LOGDSTTYPE_FILE:
                checkbak(dst, len);
                if (dst->u.file.fp) {
                    fseek(dst->u.file.fp, 0, SEEK_END);
                    if (fprintf(dst->u.file.fp, "%s", handle->bufferp) != (int)len) {
                        if (errno != 0) {
                            perror("fprintf");
                        }
                    }
                    fflush(dst->u.file.fp);
                }
                break;
            case LOGDSTTYPE_SOCK:
                if (dst->u.sock.sockfd == -1) {
                    struct hostent *host = NULL;
                    if ((host = gethostbyname(dst->u.sock.addr))) {
                        dst->u.sock.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
                        if (dst->u.sock.sockfd != -1) {
                            struct sockaddr_in addr;
                            memset(&addr, 0, sizeof(addr));
                            addr.sin_family = AF_INET;
                            addr.sin_port = htons(dst->u.sock.port);
                            addr.sin_addr = *(struct in_addr *)host->h_addr_list[0];
                            bind(dst->u.sock.sockfd, (struct sockaddr *)&addr, sizeof(addr));
                        }
                    }
                }
                if (dst->u.sock.sockfd != -1)
                    send(dst->u.sock.sockfd, handle->bufferp, len+1, 0);
                break;
            case LOGDSTTYPE_LOGCAT:
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
                break;
            case LOGDSTTYPE_SYSLOG:
                syslog(level, "%s", handle->bufferp);
                break;
            default:
                break;
        } 
    }

    handle->success_count ++;
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

    pthread_mutex_unlock(&handle->mutex);
}

void mlog(loghandler *handle, LOGLEVEL level, const char *file, 
        const char *function, long line, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(handle, level, file, function, line, format, args);
    va_end(args);
    return;
}


int log_init()
{
    if (sloger == NULL) {
        sloger = mlog_init();
        if (sloger)
            return 0;
    }
    return -1;
}

void slog(LOGLEVEL level, const char * file, const char * function, long line, const char *format, ...)
{
    if (sloger) {
        va_list args;
        va_start(args, format);
        vlog(sloger, level, file, function, line, format,  args);
        va_end(args);
    }
    return ;
}

void mlog_dump(loghandler *handle)
{
    if (handle) {
        printf("=============================================================\n");
        printf("ident: %s\n", handle->ident);
        printf("dsts: %d\n", handle->dsts_count);
        int i;
        for (i = 0; i < DEFAULT_MAX_DST; i++) {
            struct logdst *dst = handle->dsts[i];
            if (dst) {
                printf("---------------\n");
                printf("dst: %d\n", i);
                switch (dst->type)  {
                    case LOGDSTTYPE_STDOUT:
                        printf("type: stdout\n");
                        printf("format: %s\n", dst->format);
                        printf("level: %s\n", LOGLEVELSTR[dst->level]);
                        break;
                    case LOGDSTTYPE_STDERR:
                        printf("type: stdout\n");
                        printf("format: %s\n", dst->format);
                        printf("level: %s\n", LOGLEVELSTR[dst->level]);
                        break;
                    case LOGDSTTYPE_FILE:
                        printf("type: file\n");
                        printf("format: %s\n", dst->format);
                        printf("level: %s\n", LOGLEVELSTR[dst->level]);
                        printf("filename: %s\n", dst->u.file.filename);
                        printf("filemode: %u\n", dst->u.file.filemode);
                        printf("backup: %"PRIu16"\n", dst->u.file.backup);
                        printf("filesize: %"PRIu64"\n", dst->u.file.filesize);
                        break;
                    case LOGDSTTYPE_SOCK:
                        printf("type: socket\n");
                        printf("format: %s\n", dst->format);
                        printf("level: %s\n", LOGLEVELSTR[dst->level]);
                        printf("addr: %s:%d\n", dst->u.sock.addr, dst->u.sock.port);
                        break;
                    case LOGDSTTYPE_LOGCAT:
                        printf("type: logcat\n");
                        printf("format: %s\n", dst->format);
                        printf("level: %s\n", LOGLEVELSTR[dst->level]);
                        break;
                    case LOGDSTTYPE_SYSLOG:
                        printf("type: syslog\n");
                        printf("format: %s\n", dst->format);
                        printf("level: %s\n", LOGLEVELSTR[dst->level]);
                        break;
                    case LOGDSTTYPE_NONE:
                        break;
                    default:
                        printf("type: unknown\n");
                        break;
                }
            }
        }
        printf("\n");
        printf("success count: %"PRIu64"\n", handle->success_count);
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

void log_dump(void)
{
    if (sloger) {
        mlog_dump(sloger);
    }
}
