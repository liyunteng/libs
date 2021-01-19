/*
 * sock_output.c - sock_output
 *
 * Date   : 2021/01/15
 */
#include "sock_output.h"
#include "log_priv.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_SOCKADDR "127.0.0.1"
#define DEFAULT_SOCKPORT 12345


static int
sock_emit(log_output_t *output, log_handler_t *handler)
{
    sock_output_ctx *ctx = NULL;
    log_buf_t *buf       = NULL;

    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }
    ctx = (sock_output_ctx *)output->ctx;
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

    int total  = 0;
    int nsend  = 0;
    size_t len = buf_len(buf);
    while (total < len) {
        nsend =
            send(ctx->sockfd, buf->start + total, len - total, MSG_NOSIGNAL);
        if (nsend < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            } else {
                ERROR_LOG("%s://%s:%d send failed(%s)\n",
                          output->type_name,
                          ctx->addr,
                          ctx->port,
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

static int
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
        ERROR_LOG("%s gethostbyname failed(%s)\n", ctx->addr, strerror(errno));
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
        ERROR_LOG("%s://%s:%d connect(%s)\n",
                  output->type_name,
                  ctx->addr,
                  ctx->port,
                  strerror(errno));
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

log_output_t *
tcp_output_create(void)
{
    log_output_t *output = NULL;

    output = (log_output_t *)calloc(1, sizeof(log_output_t));
    if (!output) {
        ERROR_LOG("calloc failed(%s)\n", strerror(errno));
        return NULL;
    }

    output->type       = LOG_OUTTYPE_TCP;
    output->type_name  = "tcp";
    output->emit       = sock_emit;
    output->ctx_init   = sock_ctx_init;
    output->ctx_uninit = sock_ctx_uninit;
    output->dump       = sock_ctx_dump;

    return output;
}

log_output_t *
udp_output_create(void)
{
    log_output_t *output = NULL;

    output = (log_output_t *)calloc(1, sizeof(log_output_t));
    if (!output) {
        ERROR_LOG("calloc failed(%s)\n", strerror(errno));
        return NULL;
    }

    output->type       = LOG_OUTTYPE_UDP;
    output->type_name  = "udp";
    output->emit       = sock_emit;
    output->ctx_init   = sock_ctx_init;
    output->ctx_uninit = sock_ctx_uninit;
    output->dump       = sock_ctx_dump;

    return output;
}
