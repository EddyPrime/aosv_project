#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tsm_lib.h"
#include "test.h"

void child_fun(int n)
{
    unsigned char desc;
    int i, ret, fd;
    char *txt, msg[MESSAGE_SIZE] = {};
    struct group_t group_descriptor;

    tid_info("My number is %d", n);

    desc = n;
    group_descriptor.desc = desc;

    fd = open_group(&group_descriptor);
    if (fd < 0)
    {
        tid_err("open_group fd");
        goto fd_fail;
    }
    tid_info("group_dev%d opened with fd %d", n, fd);

    txt = "%ld was here";
    info("Writing %d messages", n);
    for (i = 0; i < n; i++)
    {
        sprintf(msg, txt, gettid());
        ret = send_message(fd, msg);
        if (ret < 0)
        {
            tid_err("write %d", i);
            goto fail;
        }
        tid_info("Written '%s'", msg);
    }

fail:
    close_group(fd);
fd_fail:
    tid_end();
    return;
}

int main(int argc, char *argv[])
{
    unsigned char desc;
    int fd, i, status, j;
    struct group_t group_descriptor;
    pid_t pids[CHILDREN];
    size_t msg_size;
    char msg[MESSAGE_SIZE] = {};
    ssize_t ret;

    tid_info("EXECUTING %s\n", argv[0]);

    for (i = 0; i < CHILDREN; i++)
    {
        if ((pids[i] = fork()) < 0)
        {
            tid_err("fork");
            goto fail;
        }
        else if (pids[i] == 0)
        {
            child_fun(i);
            exit(0);
        }
    }

    tid_info("Dad is going to wait %d babies", CHILDREN);
    i = 0;
    status = 0;
    while (i < CHILDREN)
    {
        tid_info("Child with PID %ld exited with status 0x%x.", (long)wait(&status), status);
        i++;
    }

    msg_size = MESSAGE_SIZE;
    memset(msg, 0, msg_size * sizeof(char));
    for (i = 0; i < CHILDREN; i++)
    {
        desc = i;
        group_descriptor.desc = desc;

        fd = open_group(&group_descriptor);
        if (fd < 0)
        {
            tid_err("open_group fd");
            goto fail;
        }
        tid_info("group_dev%d opened with fd %d", desc, fd);

        info("Reading %d messages", CHILDREN);
        for (j = 0; j < CHILDREN; j++)
        {
            ret = retrieve_message(fd, msg, msg_size);
            if (ret < 0)
            {
                tid_err("read %d", j);
                tid_info("group_dev%d closed with fd %d", desc, fd);
                close_group(fd);
                continue;
            }
            if (ret == 0)
            {
                tid_info("No more messages to read");
                break;
            }
            tid_info("Read %ld bytes: '%s'", ret, msg);
        }
        tid_info("group_dev%d closed with fd %d", desc, fd);
        close_group(fd);
    }

fail:
    tid_end();
    return 0;
}