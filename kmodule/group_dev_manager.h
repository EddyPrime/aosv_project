#pragma once

/**
 * maximum number of group devices.
 */
#define GROUP_DEV_COUNT 256 /* [0 , GROUP_DEV_COUNT - 1]. */

#define GROUP_DEVICE_NAME "group_dev"
#define GROUP_CLASS_NAME "group_dev_class"

#define GROUP_FORMAT "group_dev%d"
#define GROUP_FORMAT_LENGTH strlen(GROUP_FORMAT) + 3

/**
 * struct group_devices - struct for all group devices.
 * 
 * @used: number of used group devices
 * @major: the major number associated to all group
 * devices
 * @group_devs_list: head of the list containing all group
 * devices managing structures.
 * 
 * This struct manages all group devices.
 */
struct group_devices
{
    unsigned short used;
    unsigned int major;
    struct list_head *group_devs_list;
};

/**
 * init_group_devs() - initializes struct group devices.
 * 
 * Initializes @group_devices struct.
 * 
 * Returns:
 * 0 - ok
 * -1 - ko
 */
int init_group_devs(void);

/**
 * _get_group() - seeks for a specific group device.
 * 
 * @desc: descriptor for the group device
 * 
 * Traverses group devices list searching for a group device
 * whose minor matches @desc. Used to check whether a specific
 * group device has been already installed.
 * 
 * Returns:
 * NULL - group device not found
 * struct group_dev* - group device found
 */
struct group_dev *_get_group(int desc);

/**
 * get_group() - wrapper for _get_group.
 * 
 * @desc: descriptor for group device
 * 
 * Wrapper whihch deploys some additional checks before
 * starting the group device seek.
 * 
 * Returns:
 * NULL - group device not found
 * struct group_dev* - group device found
 */
struct group_dev *get_group(int desc);

/**
 * _install_group() - installs a group device.
 * 
 * @desc: descriptor for group device
 * 
 * The group device for @desc is installed. All structures are
 * allocated and initialized. In addition to the kernel
 * structure for managing the group device, the character device
 * itself is installed onto the machine.
 * 
 * Returns:
 * NULL - group device not found
 * struct group_dev* - group device found
 */
struct group_dev *_install_group(int desc);

/**
 * install_group() - whole group device installation process.
 * 
 * @desc: descriptor for group device
 * 
 * Wrapper for previously mentioned functions.
 * First, tries to retrieve a group device for the given
 * descriptor. In the case where no group device was found,
 * then it has to be installed. Hence, if there is enough
 * space, the function tries to install a group device.
 * 
 * Returns:
 * 0 - group device found or installed
 * -1 - no group device could be installed
 */
int install_group(struct group_t *group_desc);

/**
 * group_free() - frees a group device.
 * 
 * @dev: the group device
 * 
 * Clears group device kernel managing structures.
 * 
 * Returns:
 * void
 */
void group_free(struct group_dev *dev);

/**
 * group_free_all() - frees all group devices.
 * 
 * Clears all group devices, either their related structures,
 * including messages, and the character devices themselves.
 * Invoked when removing the module.
 * 
 * Returns:
 * void
 */
void group_free_all(void);
