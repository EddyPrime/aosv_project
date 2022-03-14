#include <linux/ioctl.h>

#define IOCTL_IDENTIFIER 's'

/**
 * IOCTL for tsm device.
 */

/* Writes to kernel the group device descriptor. */
#define IOCTL_INSTALL_GROUP _IOW(IOCTL_IDENTIFIER, 0, struct group_t *)
/* Retrieves from kernel the max_message_size module parameter. */
#define IOCTL_MAX_MESSAGE_SIZE _IOR(IOCTL_IDENTIFIER, 1, unsigned int)

/**
 * IOCTL for group devices.
 */

#define IOCTL_SLEEP_ON_BARRIER _IO(IOCTL_IDENTIFIER, 2)
#define IOCTL_AWAKE_BARRIER _IO(IOCTL_IDENTIFIER, 3)
/* Writes to kernel the group device delay. */
#define IOCTL_SET_SEND_DELAY _IOW(IOCTL_IDENTIFIER, 4, long)
#define IOCTL_REVOKE_DELAYED_MESSAGES _IO(IOCTL_IDENTIFIER, 5)
