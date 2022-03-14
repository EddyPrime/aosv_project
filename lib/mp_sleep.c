#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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
    unsigned char desc;
    int fd, i, status;
    struct group_t group_descriptor;
    pid_t pids[CHILDREN];
    size_t msg_size;
    char msg[MESSAGE_SIZE] = {};
    ssize_t ret;

    tid_info("EXECUTING %s\n", argv[0]);

    desc = 1;
    group_descriptor.desc = desc;

    fd = open_group(&group_descriptor);
    if (fd < 0)
    {
        tid_err("open_group fd");
        goto fd_fail;
    }
    tid_info("group_dev%d opened with fd %d", desc, fd);

    for (i = 0; i < CHILDREN; i++)
    {
        if ((pids[i] = fork()) < 0)
        {
            tid_err("fork");
            goto fd_fail;
        }
        else if (pids[i] == 0)
        {
            child_fun(fd);
            exit(0);
        }
    }

    sleep(5);
    awake_barrier(fd);
    tid_info("Dad has awakened babies");
    tid_info("Dad is going to wait %d babies", CHILDREN);
    i = 0;
    status = 0;
    while (i < CHILDREN)
    {
        tid_info("Child with PID %ld exited with status 0x%x.", (long)wait(&status), status);
        i++;
    }

    close_group(fd);
    tid_info("group_dev%d closed with fd %d", desc, fd);

fd_fail:
    tid_end();
    return 0;
}