/*
 * test.c - test
 *
 * Date   : 2021/03/12
 */
#include "channel.h"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct {
    struct channel_t *ch;
    int timeout;
} param_t;

typedef struct {
    int id;
    char msg[128];
} msg_t;

void *producer(void *arg)
{
    param_t *param = (param_t *)arg;
    struct channel_t *ch = param->ch;
    int timeout = param->timeout;
    int i, ret;
    msg_t msg;

    for (i = 0; i < 10; i++) {
        msg.id = i;
        snprintf(msg.msg, sizeof(msg.msg)-1, "0x%lx: this is %d",
                 (long)pthread_self(), i);

        if (timeout == 0) {
            ret = channel_push(ch, &msg);
        } else {
            ret = channel_push_timeout(ch, &msg, timeout);
        }

        if (ret != 0) {
            printf("0x%lx push failed: %s\n", (long)pthread_self(), strerror(ret));
            break;
        }
    }

    printf("0x%lx producer exit\n", (long)pthread_self());
    return (void *)0;
}

void *consumer(void *arg)
{
    param_t *param = (param_t *)arg;
    struct channel_t *ch = param->ch;
    int timeout = param->timeout;
    int ret;
    msg_t msg;

    while (1) {
        if (timeout == 0) {
            ret = channel_pop(ch, &msg);
        } else {
            ret = channel_pop_timeout(ch, &msg, timeout);
        }

        if (ret != 0) {
            printf("pop failed: %s\n", strerror(ret));
            continue;
        }

        printf("%s\n", msg.msg);

        if (strcmp(msg.msg, "quit") == 0) {
            break;
        }
    }
    printf("consumer exit\n");
    return (void *)0;
}

static void ch_send_quit(struct channel_t *ch)
{
    msg_t msg;
    msg.id = 0;
    snprintf(msg.msg, sizeof(msg.msg)-1, "quit");
    channel_push(ch, &msg);
}

int test(int np, int timeout)
{
    pthread_t c;
    pthread_t *ps;
    struct channel_t *ch;
    param_t paramp, paramc;
    int i;

    ch = channel_create(1, sizeof(msg_t));
    if (!ch) {
        printf("channel_create failed\n");
        return -1;
    }

    ps = (pthread_t *) malloc(sizeof(pthread_t)*np);
    if (!ps) {
        printf("malloc failed\n");
        return -1;
    }

    paramp.ch = ch;
    paramp.timeout = timeout;
    for (i = 0; i < np; i++) {
        if (pthread_create(&ps[i], NULL, producer, (void *)&paramp) != 0) {
            printf("pthread_create failed\n");
            return -1;
        }
    }

    paramc.ch = ch;
    paramc.timeout = 0;
    if (pthread_create(&c, NULL, consumer, (void *)&paramc) != 0) {
        printf("pthread_create failed\n");
        return -1;
    }

    for (i = 0; i < np; i++) {
        pthread_join(ps[i], NULL);
    }
    ch_send_quit(ch);
    pthread_join(c, NULL);

    channel_destroy(&ch);
    return 0;
}

int main(void)
{
    test(1, 0);
    test(5, 1);
    return 0;
}
