/*
 * proc.c - proc
 *
 * Date   : 2020/04/29
 */

#include "proc.h"

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PATH 256
#define NAME_TITLE_LEN 6 /* strlen("Name:")+1 */

static int
check_process(const char *proc_name)
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    int fd, count;
    char pathname[MAX_PATH];
    int nres = 0;

    if ((dp = opendir("/proc")) == NULL) {
        return -1;
    }

    while ((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name, &statbuf);
        if (S_ISDIR(statbuf.st_mode)) {
            if (strcmp(".", entry->d_name) == 0
                || strcmp("..", entry->d_name) == 0)
                continue;

            memset(pathname, 0, MAX_PATH);
            sprintf(pathname, "/proc/%s/status", entry->d_name);
            if (access(pathname, R_OK))
                continue;
            fd = open(pathname, O_RDONLY);
            if (fd != -1) {
                char name[MAX_PATH];
                char proc[MAX_PATH];
                count = read(fd, name, MAX_PATH);
                if (count > NAME_TITLE_LEN) {
                    sscanf(name, "Name:%s", proc);
                    if (strcmp(proc, proc_name) == 0) {
                        nres = 1;
                        break;
                    }
                }
            }
        }
    }
    closedir(dp);
    return nres;
}

int
wait_proc_exit(const char *proc_name, int timeout)
{
    if (!proc_name) {
        return -1;
    }

    while ((check_process(proc_name) > 0) && timeout > 0) {
        sleep(1);
        timeout--;
    }

    return timeout == 0 ? -1 : 0;
}

int
wait_procs_exit(const char **procs, int timeout)
{
    const char *proc_name = NULL;
    int res               = 0;
    if (!procs) {
        return -1;
    }
    proc_name = *procs;
    while (proc_name) {
        res &= wait_proc_exit(proc_name, timeout);
        proc_name = *(++procs);
    }

    return res;
}
