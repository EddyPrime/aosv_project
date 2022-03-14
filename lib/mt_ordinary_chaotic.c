#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include "tsm_lib.h"
#include "test.h"

int remaining_messages;

void *thread_fun(void *arg)
{
    int *fd;
    size_t msg_size;
    char *txt, msg[MESSAGE_SIZE] = {};
    int i, to_write, to_read, to_awake, to_sleep;
    ssize_t ret;

    tid_start();

    fd = (int *)arg;
    if (!fd || *fd < 0)
    {
        tid_err("fd");
        goto exit;
    }

    to_write = rand() % 7;
    to_read = rand() % 7;
    to_awake = rand() % 5;
    to_sleep = rand() % 2;

    remaining_messages = remaining_messages + to_write - to_read;

    tid_info("----- is going to ----");
    tid_info("- write %d messgaes   -", to_write);
    tid_info("- read %d messgaes    -", to_read);
    tid_info("- awake barrier? %d   -", !to_awake);
    tid_info("- sleep? %d           -", !!to_sleep);

    txt = "%ld was here";
    tid_info("Writing %d messages", to_write);
    for (i = 0; i < to_write; i++)
    {
        sprintf(msg, txt, gettid());
        ret = send_message(*fd, msg);
        if (ret < 0)
        {
            tid_err("write %d", i);
            goto exit;
        }
        tid_info("Written '%s'", msg);
    }

    msg_size = MESSAGE_SIZE;
    memset(msg, 0, msg_size * sizeof(char));

    tid_info("Reading %d messages", to_read);
    for (i = 0; i < to_read; i++)
    {
        ret = retrieve_message(*fd, msg, msg_size);
        if (ret < 0)
        {
            tid_err("read %d", i);
            goto exit;
        }
        if (ret == 0)
        {
            tid_info("No more messages to read");
            break;
        }
        tid_info("Read %ld bytes: '%s'", ret, msg);
    }

    if (!to_awake)
    {
        tid_info("Awaking barrier");
        awake_barrier(*fd);
    }

    if (to_sleep)
    {
        tid_info("Going to bed");
        sleep_on_barrier(*fd);
        tid_info("What a beautiful nap!");
    }

exit:
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

    remaining_messages = 0;

    for (i = 0; i < THREADS; i++)
    {
        ret = pthread_create(&tids[i], NULL, &thread_fun, (void *)&fd);
        if (ret)
        {
            tid_err("pthread_create");
            goto thread_fail;
        }
    }

    sleep(5);
    awake_barrier(fd);
    sleep(1);

    for (i = 0; i < THREADS; i++)
    {
        ret = pthread_join(tids[i], NULL);
        if (ret)
        {
            tid_err("pthread_join");
            goto thread_fail;
        }
    }

    tid_info("Thread creation and join");
    sleep(5);
    msg_size = MESSAGE_SIZE;

    info("Reading %d messages", remaining_messages);
    for (i = 0; i < remaining_messages; i++)
    {
        bytes = retrieve_message(fd, msg, msg_size);
        if (bytes < 0)
        {
            tid_err("read %d", i);
            goto thread_fail;
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