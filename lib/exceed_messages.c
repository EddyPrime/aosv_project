#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "tsm_lib.h"
#include "test.h"

int main(int argc, char *argv[])
{
    unsigned char desc;
    int fd, i;
    struct group_t group_descriptor;
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

    txt = "%d from userspace";
    info("Writing %d messages", E_MSG_TO_WRITE);
    for (i = 0; i < E_MSG_TO_WRITE; i++)
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

write_fail:
    close_group(fd);
    info("group_dev%d closed with fd %d", desc, fd);
fd_fail:
    end();
    return 0;
}