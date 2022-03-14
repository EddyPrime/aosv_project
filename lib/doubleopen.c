#include <stdio.h>
#include <stdlib.h>

#include "tsm_lib.h"
#include "test.h"

int main(int argc, char *argv[])
{
    unsigned char desc;
    int fd1, fd2;
    struct group_t group_descriptor;

    start(argv[0]);

    desc = 0;
    group_descriptor.desc = desc;

    fd1 = open_group(&group_descriptor);
    if (fd1 < 0)
    {
        err("open_group fd");
        goto fd1_fail;
    }
    info("group_dev%d opened with fd %d", desc, fd1);

    fd2 = open_group(&group_descriptor);
    if (fd2 < 0)
    {
        err("open_group fd");
        goto fd2_fail;
    }
    info("group_dev%d opened with fd %d", desc, fd2);

    mygetch();

    close_group(fd2);
    info("group_dev%d closed with fd %d", desc, fd2);
fd2_fail:
    close_group(fd1);
    info("group_dev%d closed with fd %d", desc, fd1);
fd1_fail:
    end();
    return 0;
}