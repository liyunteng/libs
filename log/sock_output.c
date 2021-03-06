/*
 * sock_output.c - sock_output
 *
 * Date   : 2021/01/15
 */

#include "sock_output.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_SOCKADDR "127.0.0.1"
#define DEFAULT_SOCKPORT 12345

struct sock_output_ctx {
    char addr[256];
    uint16_t port;
    int sockfd;
};

static int
sock_emit(struct log_output *output, struct log_handler *handler)
{
    struct sock_output_ctx *ctx = NULL;
    log_buf_t *buf              = NULL;

    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }
    ctx = (struct sock_output_ctx *)output->ctx;
    if (!ctx) {
        ERROR_LOG("ctx is NULL\n");
        return -1;
    }

    if (!handler) {
        ERROR_LOG("handler is NULL\n");
        return -1;
    }

    buf = handler->event.msg_buf;
    if (!buf) {
        ERROR_LOG("msg_buf is NULL\n");
        return -1;
    }

    if (ctx->sockfd == -1) {
        return -1;
    }

    int nsend    = 0;
    size_t total = 0;
    size_t len   = buf_len(buf);
    while (total < len) {
        nsend =
            send(ctx->sockfd, buf->start + total, len - total, MSG_NOSIGNAL);
        if (nsend < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            } else {
                ERROR_LOG("%s://%s:%d send failed: (%s)\n",
                          output->priv->type_name, ctx->addr, ctx->port,
                          strerror(errno));
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

static void
sock_ctx_dump(struct log_output *output)
{
    if (output) {
        DUMP_LOG("type: %s\n", output->priv->type_name);
        struct sock_output_ctx *ctx = (struct sock_output_ctx *)output->ctx;
        if (ctx) {
            DUMP_LOG("addr: %s:%d\n", ctx->addr, ctx->port);
        }
        dump_statstic(output);
    }
}

static int
sock_ctx_init(struct log_output *output, va_list ap)
{
    struct sock_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    if (output->ctx == NULL) {
        output->ctx =
            (struct sock_output_ctx *)calloc(1, sizeof(struct sock_output_ctx));
        if (!output->ctx) {
            ERROR_LOG("calloc failed: (%s)\n", strerror(errno));
            goto failed;
        }
        ((struct sock_output_ctx *)(output->ctx))->sockfd = -1;
    }
    ctx = (struct sock_output_ctx *)output->ctx;

    char *addr_str = va_arg(ap, char *);
    strncpy(ctx->addr, addr_str, sizeof(ctx->addr));

    unsigned port = va_arg(ap, unsigned);
    ctx->port     = port;

    if (ctx->sockfd != -1) {
        close(ctx->sockfd);
        ctx->sockfd = -1;
    }

    struct hostent *host = gethostbyname(ctx->addr);
    if (host == NULL) {
        ERROR_LOG("%s gethostbyname failed: (%s)\n", ctx->addr,
                  strerror(errno));
        goto failed;
    }

    if (output->priv->type == LOG_OUTTYPE_TCP) {
        ctx->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        ctx->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    }
    if (ctx->sockfd < 0) {
        ERROR_LOG("socket failed: (%s)\n", strerror(errno));
        goto failed;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(ctx->port);
    addr.sin_addr   = *(struct in_addr *)(host->h_addr_list[0]);
    if (connect(ctx->sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ERROR_LOG("%s://%s:%d connect failed: (%s)\n", output->priv->type_name,
                  ctx->addr, ctx->port, strerror(errno));
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

static void
sock_ctx_uninit(struct log_output *output)
{
    struct sock_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return;
    }
    ctx = (struct sock_output_ctx *)output->ctx;
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

struct log_output_priv tcp_output_priv = {
    .type       = LOG_OUTTYPE_TCP,
    .type_name  = "tcp",
    .emit       = sock_emit,
    .ctx_init   = sock_ctx_init,
    .ctx_uninit = sock_ctx_uninit,
    .dump       = sock_ctx_dump,
};

struct log_output_priv udp_output_priv = {
    .type       = LOG_OUTTYPE_UDP,
    .type_name  = "udp",
    .emit       = sock_emit,
    .ctx_init   = sock_ctx_init,
    .ctx_uninit = sock_ctx_uninit,
    .dump       = sock_ctx_dump,
};
