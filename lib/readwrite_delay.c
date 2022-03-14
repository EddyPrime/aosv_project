#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "tsm_lib.h"
#include "test.h"

int main(int argc, char *argv[])
{
    unsigned char desc;
    int fd, i;
    struct group_t group_descriptor;
    long delay;
    size_t msg_size;
    char *txt, msg[MESSAGE_SIZE] = {};
    ssize_t ret;

    start(argv[0]);

    desc = 0;
    group_descriptor.desc = desc;

    fd = open_group(&group_descriptor);
    if (fd < 0)
    {
        err("open_group fd");
        goto fd_fail;
    }
    info("group_dev%d opened with fd %d", desc, fd);

    delay = 1000 * 5;
    delay = -1;
    ret = set_send_delay(fd, delay);
    if (ret < 0)
    {
        err("delay %ld", delay);
        goto write_fail;
    }
    info("Set delay %ld", delay);

    txt = "%d from userspace";
    info("Writing %d messages", MSG_TO_WRITE);
    for (i = 0; i < MSG_TO_WRITE; i++)
    {
        sprintf(msg, txt, i);
        ret = send_message(fd, msg);
        if (ret < 0)
        {
            err("write %d", i);
            goto write_fail;
        }
        info("Written '%s'", msg);
    }

    sleep(2);

    msg_size = MESSAGE_SIZE;
    memset(msg, 0, msg_size * sizeof(char));
    info("Reading %d messages", MSG_TO_READ);
    for (i = 0; i < MSG_TO_READ; i++)
    {
        ret = retrieve_message(fd, msg, msg_size);
        if (ret < 0)
        {
            tid_err("read %d", i);
            goto write_fail;
        }
        if (ret == 0)
        {
            tid_info("No more messages to read");
            break;
        }
        tid_info("Read %ld bytes: '%s'", ret, msg);
    }

write_fail:
    close_group(fd);
    info("group_dev%d closed with fd %d", desc, fd);
fd_fail:
    end();
    return 0;
}
