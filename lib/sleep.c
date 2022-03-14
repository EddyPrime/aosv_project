#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "tsm_lib.h"
#include "test.h"

void child_fun(int fd)
{
    tid_start();
    tid_info("Going to bed");
    if (sleep_on_barrier(fd) < 0)
    {
        tid_err("Cannot sleep");
    }
    tid_info("What a beautiful nap!");
    close_group(fd);
    tid_end();
    return;
}

int main(int argc, char *argv[])
{
    unsigned char desc = 1;
    int fd, status;
    struct group_t group_descriptor;
    pid_t pid;

    tid_info("EXECUTING %s\n", argv[0]);

    group_descriptor.desc = desc;

    fd = open_group(&group_descriptor);
    if (fd < 0)
    {
        tid_err("open_group fd");
        goto fd_fail;
    }
    tid_info("group_dev%d opened with fd %d", desc, fd);

    pid = fork();
    if (pid < 0)
    {
        tid_err("fork");
        goto fork_fail;
    }
    if (pid == 0)
    {
        child_fun(fd);
        exit(0);
    }

    sleep(2);
    awake_barrier(fd);
    tid_info("Barrier awakened");
    sleep(2);
    wait(&status);

fork_fail:
    close_group(fd);
    tid_info("group_dev%d closed with fd %d", desc, fd);
fd_fail:
    tid_end();
    return 0;
}