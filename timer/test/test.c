/*
 * test.c - test
 *
 * Date   : 2020/04/30
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>

#include "timer.h"

void second_add(void* data)
{
    struct tm *tm = (struct tm*)data;
    tm->tm_sec++;
    if(tm->tm_sec == 60) {
        tm->tm_min++;
        tm->tm_sec = 0;
    }
    if(tm->tm_min == 60) {
        tm->tm_hour++;
        tm->tm_min = 0;
    }
    if(tm->tm_hour == 24) {
        tm->tm_mday++;
        tm->tm_hour = 0;
    }
}

void time_print(void* data)
{
    struct tm *t = (struct tm*)data;

    fprintf(stderr, "%04d-%02d-%02d %02d:%02d:%02d\t", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

    time_t ti = time(NULL);
    struct tm tt;
    localtime_r(&ti, &tt);
    printf("local time: %04d-%02d-%02d %02d:%02d:%02d\n", tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour, tt.tm_min, tt.tm_sec);
}

int main(int argc, char* argv[])
{

    timer_ctx_t ctx;
    time_t t = time(NULL);
    if(timer_init(&ctx) == NULL) {
        fprintf(stderr, "init failed!\n");
        return -1;
    }

    struct tm tm;
    localtime_r(&t, &tm);
    printf("local time: %04d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    timer_register(&ctx, second_add, &tm, 1000, 1000, 0);

    timer_register(&ctx, time_print, &tm, 1100, 100, 0);
    int c = 6000;
    while(c-- > 0) {
        timer_run(&ctx);
        poll(NULL, 0, 10);
    }

    printf("Clean up\n");
    timer_cleanup(&ctx);
    return 0;
}
