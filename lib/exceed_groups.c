#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "tsm_lib.h"

#define TO_INSTALL 260

int mygetch(void)
{
    int ch;
    struct termios oldt, newt;
    printf("Press any key to continue ..\n");
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    return ch;
}

int main(int argc, char *argv[])
{
    int i;
    int gfd[TO_INSTALL] = {};
    unsigned char desc[TO_INSTALL] = {};
    struct group_t group_descriptor;

    start(argv[0]);

    for (i = 0; i < TO_INSTALL; i++)
    {
        desc[i] = i;
        group_descriptor.desc = desc[i];
        gfd[i] = -1;
        gfd[i] = open_group(&group_descriptor);
        if (gfd[i] < 0)
        {
            err("open_group fd");
            break;
        }
        info("group_dev%d opened with fd %d", desc[i], gfd[i]);
    }

    //mygetch();

    for (i = 0; i < TO_INSTALL; i++)
    {
        if (gfd[i] < 0)
        {
            break;
        }
        close_group(gfd[i]);
        info("group_dev%d closed with fd %d", desc[i], gfd[i]);
    }

    end();
    return 0;
}