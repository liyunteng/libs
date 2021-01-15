/*
 * sock_output.c - sock_output
 *
 * Date   : 2021/01/15
 */
#include "log_priv.h"
#include "sock_output.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <string.h>
#include <errno.h>
#include <unistd.h>


int
sock_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len)
{
    sock_output_ctx *ctx = NULL;

    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }
    ctx = (sock_output_ctx *)output->ctx;
    if (!ctx) {
        ERROR_LOG("ctx is NULL\n");
        return -1;
    }

    if (ctx->sockfd == -1) {
        return -1;
    }

    int total = 0;
    int nsend = 0;
    while (total < len) {
        nsend = send(ctx->sockfd, buf + total, len - total, MSG_NOSIGNAL);
        if (nsend < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            } else {
                ERROR_LOG("send failed(%s)\n", strerror(errno));
                close(ctx->sockfd);
                ctx->sockfd = -1;
                break;
            }
        } else if (nsend == 0) {
            ERROR_LOG("sock closed\n");
            close(ctx->sockfd);
            ctx->sockfd = -1;
            break;
        } else {
            total += nsend;
        }
    }
    return total;
}

void
sock_ctx_dump(log_output_t *output)
{
    if (output) {
        printf("type: %s\n", output->type_name);
        sock_output_ctx *ctx = (sock_output_ctx *)output->ctx;
        if (ctx) {
            printf("addr: %s:%d\n", ctx->addr, ctx->port);
        }
        dump_statstic(output);
    }
}

int
sock_ctx_init(log_output_t *output, va_list ap)
{
    sock_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    if (output->ctx == NULL) {
        output->ctx = (sock_output_ctx *)calloc(1, sizeof(sock_output_ctx));
        if (!output->ctx) {
            ERROR_LOG("calloc failed(%s)\n", strerror(errno));
            goto failed;
        }
        ((sock_output_ctx *)(output->ctx))->sockfd = -1;
    }
    ctx = (sock_output_ctx *)output->ctx;

    char *addr_str = va_arg(ap, char *);
    if (addr_str && strlen(addr_str) > 0) {
        strncpy(ctx->addr, addr_str, strlen(addr_str) + 1);
    } else {
        strncpy(ctx->addr, DEFAULT_SOCKADDR, strlen(DEFAULT_SOCKADDR) + 1);
    }

    unsigned port = va_arg(ap, unsigned);
    if (port > 0 && port < 65535) {
        ctx->port = port;
    } else {
        ctx->port = DEFAULT_SOCKPORT;
    }

    if (ctx->sockfd != -1) {
        close(ctx->sockfd);
        ctx->sockfd = -1;
    }

    struct hostent *host = gethostbyname(ctx->addr);
    if (host == NULL) {
        ERROR_LOG("gethostbyname failed(%s)\n", strerror(errno));
        goto failed;
    }

    if (output->type == LOG_OUTTYPE_TCP) {
        ctx->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        ctx->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    }
    if (ctx->sockfd < 0) {
        ERROR_LOG("socket failed(%s)\n", strerror(errno));
        goto failed;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(ctx->port);
    addr.sin_addr   = *(struct in_addr *)(host->h_addr_list[0]);
    if (connect(ctx->sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ERROR_LOG("connect(%s)\n", strerror(errno));
        goto failed;
    }

    return 0;

failed:
    if (ctx != NULL) {
        if (ctx->sockfd != -1) {
            close(ctx->sockfd);
            ctx->sockfd = -1;
        }
        free(ctx);
        ctx = NULL;
    }
    return -1;
}

void
sock_ctx_uninit(log_output_t *output)
{
    sock_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return;
    }
    ctx = (sock_output_ctx *)output->ctx;
    if (ctx == NULL) {
        return;
    }

    if (ctx->sockfd != -1) {
        close(ctx->sockfd);
        ctx->sockfd = -1;
    }
    free(ctx);
    ctx         = NULL;
    output->ctx = NULL;
}
