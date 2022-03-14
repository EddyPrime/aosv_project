#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "tsm_lib.h"
#include "test.h"

#define ITERATIONS 2

void *thread_fun(void *arg)
{
    int *fd;
    char *txt, msg[MESSAGE_SIZE] = {};
    int i;
    ssize_t ret;

    tid_start();
    fd = (int *)arg;
    if (!fd || *fd < 0)
    {
        tid_err("fd");
        goto fail;
    }

    txt = "%ld was here";
    info("Writing %d messages", ITERATIONS + 1);
    for (i = 0; i < ITERATIONS + 1; i++)
    {
        sprintf(msg, txt, gettid());
        ret = send_message(*fd, msg);
        if (ret < 0)
        {
            tid_err("write %d", i);
            goto fail;
        }
        tid_info("Written '%s'", msg);
    }

fail:
    tid_end();
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    unsigned char desc;
    int fd, i, ret;
    struct group_t group_descriptor;
    pthread_t tids[THREADS];
    size_t msg_size;
    char msg[MESSAGE_SIZE] = {};
    ssize_t bytes;

    tid_info("EXECUTING %s\n", argv[0]);

    desc = 0;
    group_descriptor.desc = desc;

    fd = open_group(&group_descriptor);
    if (fd < 0)
    {
        tid_err("open_group fd");
        goto fd_fail;
    }
    tid_info("group_dev%d opened with fd %d", desc, fd);

    for (i = 0; i < THREADS; i++)
    {
        ret = pthread_create(&tids[i], NULL, &thread_fun, (void *)&fd);
        if (ret)
        {
            tid_err("pthread_create");
            goto thread_fail;
        }
    }

    for (i = 0; i < THREADS; i++)
    {
        ret = pthread_join(tids[i], NULL);
        if (ret)
        {
            tid_err("pthread_join");
            goto thread_fail;
        }
    }

    msg_size = MESSAGE_SIZE;
    info("Reading %d messages", THREADS * ITERATIONS);
    for (i = 0; i < THREADS * ITERATIONS; i++)
    {
        bytes = retrieve_message(fd, msg, msg_size);
        if (bytes < 0)
        {
            tid_err("read %d", i);
            continue;
        }
        if (bytes == 0)
        {
            tid_info("No more messages to read");
            break;
        }
        tid_info("Read %ld bytes: '%s'", bytes, msg);
    }

thread_fail:
    close_group(fd);
    tid_info("group_dev%d closed with fd %d", desc, fd);
fd_fail:
    tid_end();
    return 0;
}