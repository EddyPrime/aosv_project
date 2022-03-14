#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>

#include "tsm_lib.h"
#include "../kmodule/ioctl.h"

int open_group(struct group_t *group_descriptor)
{
    int ret, fd, i;
    char *group_dev_name;

    /* Check for group descriptor. */
    if (!group_descriptor)
    {
        errno = -EINVAL;
        ret = -1;
        goto exit;
    }

    if (group_descriptor->desc < 0)
    {
        errno = -EINVAL;
        ret = -1;
        goto exit;
    }

    //MAY ADD CHECK ON DESC UPPERBOUND

    /* Open tsm dev.
       It will mediate the opening of a group device. */
    fd = open(TSM_DEV, O_RDWR);
    if (fd < 0)
    {
        err("%s open", TSM_DEV);
        errno = -ENODEV;
        ret = -1;
        goto exit;
    }
    dbg("%s opened with fd %d", TSM_DEV, fd);

    /* Invoke IOCTL call to open, and to install if needed, a 
       group device. */
    ret = ioctl(fd, IOCTL_INSTALL_GROUP, group_descriptor); //VALGRIND ERROR
    if (ret < 0)
    {
        err("IOCTL_INSTALL_GROUP");
        close(fd); /* Close tsm device. */
        errno = -ENOSYS;
        ret = -1;
        goto exit;
    }

    if (!max_message_size)
    {
        ret = ioctl(fd, IOCTL_MAX_MESSAGE_SIZE, &max_message_size);
        if (ret < 0)
        {
            err("IOCTL_INSTALL_GROUP");
            close(fd); /* Close tsm device. */
            errno = -ENOSYS;
            ret = -1;
            goto exit;
        }
        info("max_message_size: %u", max_message_size);
    }

    close(fd); /* Task completed. Close tsm device. */
    dbg("%s closed with fd %d", TSM_DEV, fd);

    /* Allocate and forge group device name. */
    group_dev_name = malloc(GROUP_DEV_LENGTH * sizeof(char));
    if (!group_dev_name)
    {
        err("group_dev_name");
        errno = -ENOMEM;
        ret = -1;
        goto exit;
    }
    dbg("group_dev_name allocated");
    sprintf(group_dev_name, GROUP_DEV, group_descriptor->desc);
    dbg("%s after sprintf", group_dev_name);

    /* Waiting for udev to create the file related to the group
       device. If no time tolerance, file  may not be found. */
    dbg("waiting udev");
    i = 0;
    while (i < ATTEMPTS)
    {
        /* Try to open the group device. If file descriptor is 
           non-negative, open was successful. */
        fd = open(group_dev_name, O_RDWR);
        if (fd >= 0)
        {
            dbg("congrats udev");
            ret = fd; /* Return the file descriptor */
            goto udev_exit;
        }
        sleep(SLEEP_TIME);
        i++;
    }

    /* Waiting time has expired. No open was successful. */
    dbg("waiting udev expired");
    ret = -1;
udev_exit:
    free(group_dev_name); /* Free group device name. */
exit:
    return ret;
}

ssize_t send_message(int fd, char *msg)
{
    ssize_t ret;
    size_t length;
    char victim;

    /* Check validity of file descriptor. */
    if (fd < 0)
    {
        err("fd");
        errno = -EINVAL;
        ret = -1;
        goto exit;
    }

    if (!max_message_size)
    {
        err("max_message_size");
        errno = -EIO;
        ret = -1;
        goto exit;
    }

    if (!msg)
    {
        err("msg");
        errno = -EINVAL;
        ret = -1;
        goto exit;
    }

    victim = msg[max_message_size];
    msg[max_message_size] = 0;
    length = strlen(msg);
    msg[max_message_size] = victim;

    /* Check message length. */
    if (length <= 0)
    {
        err("length");
        errno = -EINVAL;
        ret = -1;
        goto exit;
    }

    dbg("write %ld bytes to %d ", length, fd);
    /* Write a message. */
    ret = write(fd, msg, length);
exit:
    return ret;
}

ssize_t retrieve_message(int fd, char *buf, size_t length)
{
    ssize_t ret;

    /* Check validity of file descriptor. */
    if (fd < 0)
    {
        err("fd");
        errno = -EINVAL;
        ret = -1;
        goto exit;
    }

    /* Check message length. */
    if (length < 0)
    {
        err("length");
        errno = -EINVAL;
        ret = -1;
        goto exit;
    }

    dbg("read %ld bytes from %d ", length, fd);
    /* Read a message. */
    ret = read(fd, buf, length);
exit:
    return ret;
}

int sleep_on_barrier(int fd)
{
    int ret;

    /* Check validity of file descriptor. */
    if (fd < 0)
    {
        err("fd");
        errno = -EINVAL;
        ret = -1;
        goto exit;
    }

    dbg("IOCTL_SLEEP_ON_BARRIER");
    /* Invoke right IOCTL call. */
    ret = ioctl(fd, IOCTL_SLEEP_ON_BARRIER);
exit:
    return ret;
}

int awake_barrier(int fd)
{
    int ret;

    /* Check validity of file descriptor. */
    if (fd < 0)
    {
        err("fd");
        errno = -EINVAL;
        ret = -1;
        goto exit;
    }

    dbg("IOCTL_AWAKE_BARRIER");
    /* Invoke right IOCTL call. */
    ret = ioctl(fd, IOCTL_AWAKE_BARRIER);
exit:
    return ret;
}

int set_send_delay(int fd, long delay)
{
    int ret;

    /* Check validity of file descriptor. */
    if (fd < 0)
    {
        err("fd");
        errno = -EINVAL;
        ret = -1;
        goto exit;
    }

    if (delay < 0)
    {
        err("delay");
        errno = -EINVAL;
        ret = -1;
        goto exit;
    }

    dbg("IOCTL_SET_SEND_DELAY with delay %ld", delay);
    /* Invoke right IOCTL call with delay as argument. */
    ret = ioctl(fd, IOCTL_SET_SEND_DELAY, delay); //VALGRIND ERROR
exit:
    return ret;
}

int revoke_delayed_messages(int fd)
{
    int ret;

    /* Check validity of file descriptor. */
    if (fd < 0)
    {
        err("fd");
        errno = -EINVAL;
        ret = -1;
        goto exit;
    }

    dbg("IOCTL_REVOKE_DELAYED_MESSAGES");
    /* Invoke right IOCTL call. */
    ret = ioctl(fd, IOCTL_REVOKE_DELAYED_MESSAGES);
exit:
    return ret;
}

void close_group(int fd)
{
    /* Check validity of file descriptor */
    if (fd < 0)
    {
        err("fd");
        errno = -EINVAL;
        return;
    }

    /* Close file descriptor. */
    close(fd);
    dbg("closed fd %d", fd);
    return;
}