#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>

#include "../common.h"
#include "kern.h"
#include "tsm.h"
#include "ioctl.h"
#include "group_dev_manager.h"

int major = TSM_MAJOR;
int minor = 0;

struct cdev tsm_cdev;
struct class *tsm_dev_class;

/* Initialize and retrieve module parameters.
   Export to have them available around the moudle. */

unsigned int max_message_size = DEFAULT_MAX_MESSAGE_SIZE;
module_param(max_message_size, uint, 0644);
MODULE_PARM_DESC(max_message_size, "The maximum size of a message");
EXPORT_SYMBOL(max_message_size);

unsigned int max_storage_size = DEFAULT_MAX_STORAGE_SIZE;
module_param(max_storage_size, uint, 0644);
MODULE_PARM_DESC(max_storage_size, "The maximum size of the storage");
EXPORT_SYMBOL(max_storage_size);

/* Associate specialized file operations. */
struct file_operations tsm_dev_fops = {
    .owner = THIS_MODULE,
    .open = tsm_dev_open,
    .release = tsm_dev_release,
    .compat_ioctl = tsm_dev_ioctl,
    .unlocked_ioctl = tsm_dev_ioctl};

static int tsm_dev_open(struct inode *inode, struct file *filp)
{
    dbg_start();
    dbg_end();
    return 0;
}

static int tsm_dev_release(struct inode *inode, struct file *filp)
{
    dbg_start();
    dbg_end();
    return 0;
}

static long tsm_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long ret;
    struct group_t group_desc;

    dbg_start();

    /* IOCTL cases: */
    /* First case:  a thread wants to to install a group. */
    /* Second case: userspace library needs maximum message size value. */
    switch (cmd)
    {
    case IOCTL_INSTALL_GROUP:
        info("IOCTL_INSTALL_GROUP\n");
        /* Get group descriptor from userspace. */
        if (copy_from_user(&group_desc, (struct group_t *)arg, sizeof(struct group_t)))
        {
            err("copy_from_user\n");
            ret = -1;
            goto exit;
        }
        /* Install the group device. */
        ret = install_group(&group_desc);
        goto exit;
    case IOCTL_MAX_MESSAGE_SIZE:
        info("IOCTL_INSTALL_GROUP\n");
        /* Provide userspace with maximum message size. */
        if (copy_to_user((unsigned int *)arg, &max_message_size, sizeof(unsigned int)))
        {
            err("copy_to_user\n");
            ret = -1;
            goto exit;
        }
        ret = 0;
        goto exit;
    }

exit:
    dbg_end();
    return ret;
}

static int __init tsm_init(void)
{
    int ret;
    dev_t dev;

    info_start();
    info("max_message_size: %u\n", max_message_size);
    info("max_storage_size: %u\n", max_storage_size);
    info("DEBUG: %d\n", DEBUG);

    /* Try to allocate character device region according to a specified
       major. If no major is specified, dynamically allocate region. */
    if (major)
    {
        dev = MKDEV(major, minor);
        ret = register_chrdev_region(dev, 1, TSM_NAME);
        dbg("register_chrdev_region for major %d and minor %d\n", major, minor);
    }
    else
    {
        ret = alloc_chrdev_region(&dev, minor, 1, TSM_NAME);
        major = MAJOR(dev);
        dbg("alloc_chrdev_region for major %d and minor %d\n", major, minor);
    }

    /* Registration failed. */
    if (ret < 0)
    {
        err("major %d registration failed\n", major);
        ret = -1;
        goto exit;
    }

    /* Create the class tsm device belongs to. */
    if (!tsm_dev_class)
    {
        tsm_dev_class = class_create(THIS_MODULE, TSM_CLASS_NAME);
    }
    dbg("tsm_dev_class initialized\n");

    /* Initialize char dev structure. 
       Associate specific file operations to the tsm device.
       Create the tsm device. */
    cdev_init(&tsm_cdev, &tsm_dev_fops);
    cdev_add(&tsm_cdev, dev, 1);
    device_create(tsm_dev_class, NULL, MKDEV(major, minor), NULL, TSM_NAME);

    info("tsm registered with major %d and minor %d\n", major, minor);
    ret = 0;

exit:
    info_end();
    return ret;
}

static void __exit tsm_exit(void)
{
    info_start();
    group_free_all(); /* Now free all group devices. */
    dbg("cleanup_groups\n");
    device_destroy(tsm_dev_class, MKDEV(major, minor)); /* Destroy tsm device. */
    dbg("device_destroy\n");
    class_destroy(tsm_dev_class); /* Destroy tsm class. */
    dbg("class_destroy\n");
    cdev_del(&tsm_cdev); /* Destroy char dev structure. */
    dbg("cdev del\n");
    unregister_chrdev_region(MKDEV(major, minor), 1); /* Unregister the major. */
    dbg("unregister_chrdev_region\n");
    info_end();
}

module_init(tsm_init); /* Specify function to be invoked when moudle is inserted. */
module_exit(tsm_exit); /* Specify function to be invoked when moudle is removed. */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Edoardo Liberati <liberati.1754399@studenti.uniroma1.it>");
MODULE_VERSION("1.0");
