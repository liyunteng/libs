/*
 * proc.c - proc
 *
 * Date   : 2020/04/29
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "proc.h"

#define MAX_PATH 256
#define NAME_TITLE_LEN 6        /* strlen("Name:")+1 */
int check_process(const char *proc_name)
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
            if (strcmp(".", entry->d_name) == 0 ||
                strcmp("..", entry->d_name) == 0)
                continue;

            memset(pathname, 0, MAX_PATH);
            sprintf(pathname, "/proc/%s/status", entry->d_name);
            if (access(pathname, R_OK))
                continue;
            fd = open(pathname, O_RDONLY);
            if (fd != -1) {
                char name[MAX_PATH];
                char proc[MAX_PATH];
                count =read (fd, name, MAX_PATH);
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

bool wait_proc_exit(const char *proc_name, int timeout)
{
    if (!proc_name) {
        return false;
    }

    while ((check_process(proc_name) > 0) && timeout > 0) {
        sleep(1);
        timeout--;
    }

    return timeout != 0;
}

bool wait_procs_exit(const char **procs, int timeout)
{
    const char *proc_name = NULL;
    bool res = true;
    if (!procs) {
        return false;
    }
    proc_name = *procs;
    while (proc_name) {
        res &= wait_proc_exit(proc_name, timeout);
        proc_name = *(++procs);
    }

    return res;
}