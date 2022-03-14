#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include "test.h"

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