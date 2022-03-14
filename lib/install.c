#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "tsm_lib.h"
#include "test.h"

int main(int argc, char *argv[])
{
    unsigned char desc;
    int fd;
    struct group_t group_descriptor;

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

    mygetch();

    close_group(fd);
    info("group_dev%d closed with fd %d", desc, fd);

fd_fail:
    end();
    return 0;
}