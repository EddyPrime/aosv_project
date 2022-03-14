#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "tsm_lib.h"
#include "test.h"

#define TO_INSTALL 4

int main(int argc, char *argv[])
{
    int i;
    int fd[TO_INSTALL] = {};
    unsigned char desc[TO_INSTALL] = {};
    struct group_t group_descriptor;

    start(argv[0]);

    for (i = 0; i < TO_INSTALL; i++)
    {
        desc[i] = i;
        group_descriptor.desc = desc[i];
        fd[i] = open_group(&group_descriptor);
        if (fd[i] < 0)
        {
            err("open_group fd");
            goto fd_fail;
        }
        info("group_dev%d opened with fd %d", desc[i], fd[i]);
    }

    mygetch();

    for (i = 0; i < TO_INSTALL; i++)
    {
        close_group(fd[i]);
        info("group_dev%d closed with fd %d", desc[i], fd[i]);
    }

fd_fail:
    end();
    return 0;
}