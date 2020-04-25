/*
 * daemon.c - daemon
 *
 * Date   : 2020/04/25
 */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "daemon.h"

#define ENV_NO_DAEMON "NO_DAEMON"
#define ENV_DAEMON_PATH "DEV_PATH"
#define DAEMON_LOG_FILE "daemon.log"
#define DAEMON_NAME_LEN 256
#define DAEMON_COMMON_LEN 1024

int
Daemon(void)
{
    char *no_daemon = NULL;
    char daemon_name[DAEMON_NAME_LEN];
    pid_t pid;
    int fd;

    /* start as daemon as default, but let environment variable override */
    no_daemon = getenv(ENV_NO_DAEMON);
    if (no_daemon && strcasecmp(no_daemon, "yes") == 0) {
        return 0;
    }

    if ((pid = fork()) < 0) {
        perror("first fork");
        exit(1);
    } else if (pid != 0) {
        /* parent exit */
        exit(0);
    }

    setsid();

    if ((pid = fork()) < 0) {
        perror("second fork");
        exit(1);
    } else if (pid != 0) {
        /* second parent exit */
        exit(0);
    }

    setsid();
    umask(0);
    close(0);

    fd = open("/dev/null", O_RDONLY);
    if (fd == -1) {
        perror("reopen stdin");
        exit(1);
    }

    close(1);
    memset(daemon_name, 0, sizeof(daemon_name));
    no_daemon = getenv(ENV_DAEMON_PATH);
    if (no_daemon) {
        strcpy(daemon_name, no_daemon);
    }
    strcat(daemon_name, DAEMON_LOG_FILE);
    fd = open(daemon_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("reopen stdout");
        exit(1);
    }

    close(2);
    fd = dup(1);
    if (fd == -1) {
        perror("reopen stderr");
        exit(1);
    }

    return 0;
}

void
daemon_sem_wait (sem_t *sem)
{
    int rc = 0;
    while (1) {
        rc = sem_wait(sem);
        if (rc == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                break;
            }
        } else {
            break;
        }
    }
}
