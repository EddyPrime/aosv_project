#include <linux/slab.h>

#include "../common.h"
#include "kern.h"
#include "group_dev_manager.h"
#include "group_dev.h"

struct group_devices *group_devs;
struct class *group_dev_class;

void group_devs_list_print(struct list_head *list)
{
    int i = 0;
    struct group_dev *tmp;
    struct list_head *pos;

    dbg_start();

    if (!list)
    {
        ref_err("list");
        goto exit;
    }

    list_for_each(pos, list)
    {
        tmp = list_entry(pos, struct group_dev, list);
        dbg("%d, group_dev%d - messages_number : %d\n", i++, tmp->minor, tmp->messages_number);
    }

exit:
    dbg_end();
    return;
}

int init_group_devs(void)
{
    int ret;

    dbg_start();
    ret = -1;

    /* Allocate group_devs. */
    group_devs = kzalloc(sizeof(struct group_devices), GFP_KERNEL);
    if (!group_devs)
    {
        kzalloc_err("group_devs");
        goto exit;
    }
    dbg("group_devs allocated\n");

    /* Allocate and initialize group_devs_list. */
    group_devs->group_devs_list = kmalloc(sizeof(struct list_head), GFP_KERNEL);
    if (!group_devs->group_devs_list)
    {
        kmalloc_err("group_devs->group_devs_list");
        goto list_fail;
    }
    INIT_LIST_HEAD(group_devs->group_devs_list);
    dbg("group_devs_list allocated\n");

    ret = 0;
    goto exit;

list_fail:
    kfree(group_devs);
exit:
    dbg_end();
    return ret;
}

struct group_dev *_get_group(int desc)
{
    struct group_dev *gd;

    dbg_start();

    /* Scan the list searching for a match of desc. */
    list_for_each_entry(gd, group_devs->group_devs_list, list)
    {
        if (gd->minor == desc)
        {
            dbg("group_dev found for desc %d\n", desc);
            goto exit;
        }
    }

    dbg("no group_dev found for desc %d\n", desc);
    gd = NULL;

exit:
    dbg_end();
    return gd;
}

struct group_dev *get_group(int desc)
{
    dbg_start();

    /* Check group_devs_list. */
    if (!group_devs->group_devs_list || list_empty(group_devs->group_devs_list))
    {
        warn("cannot reference or empty group_devs_list\n");
        return NULL;
    }

    dbg_end();

    /* Retrieve a group device depending on desc. */
    return _get_group(desc);
}

struct group_dev *_install_group(int desc)
{
    int err, minor, major;
    char *device_name;
    dev_t dev;
    struct group_dev *new_group_dev;

    dbg_start();

    minor = desc;
    major = group_devs->major;

    /* Allocate group device name. */
    device_name = kmalloc(GROUP_FORMAT_LENGTH * sizeof(char), GFP_KERNEL);
    if (!device_name)
    {
        kzalloc_err("device_name");
        goto dev_name_fail;
    }
    dbg("device_name allocated\n");

    /* Forge group device name. */
    sprintf(device_name, GROUP_FORMAT, minor);
    dbg("device_name is %s\n", device_name);
    dbg("initial major : %d minor : %d", major, minor);

    /* Allocate structure for group device to be installed. */
    new_group_dev = kzalloc(sizeof(struct group_dev), GFP_KERNEL);
    if (!new_group_dev)
    {
        kzalloc_err("new_group_dev");
        goto dev_alloc_fail;
    }
    dbg("new_group_dev allocated\n");

    /* Allocate and initialize messages list. */
    new_group_dev->message_list = kmalloc(sizeof(struct list_head), GFP_KERNEL);
    if (!new_group_dev->message_list)
    {
        kmalloc_err("group_dev->message_list");
        goto msg_list_fail;
    }
    INIT_LIST_HEAD(new_group_dev->message_list);
    dbg("new_group_dev->message_list allocated\n");

    /* Allocate and initialize message semaphore. */
    new_group_dev->message_sem = kmalloc(sizeof(struct semaphore), GFP_KERNEL);
    if (!new_group_dev->message_sem)
    {
        kmalloc_err("group_dev->message_sem");
        goto msg_sem_fail;
    }
    sema_init(new_group_dev->message_sem, 1);
    dbg("new_group_dev->message_sem allocated\n");

    /* Allocate and initialize pending messages list. */
    new_group_dev->pending_list = kmalloc(sizeof(struct list_head), GFP_KERNEL);
    if (!new_group_dev->pending_list)
    {
        kmalloc_err("group_dev->pending_list");
        goto pnd_list_fail;
    }
    INIT_LIST_HEAD(new_group_dev->pending_list);
    dbg("new_group_dev->pending_list allocated\n");

    /* Allocate and initialize pending semaphore. */
    new_group_dev->pending_sem = kmalloc(sizeof(struct semaphore), GFP_KERNEL);
    if (!new_group_dev->pending_sem)
    {
        kmalloc_err("group_dev->pending_sem");
        goto pnd_sem_fail;
    }
    sema_init(new_group_dev->pending_sem, 1);
    dbg("new_group_dev->pending_sem allocated\n");

    /* Allocate and initialize workqueue semaphore. */
    new_group_dev->wq_sem = kmalloc(sizeof(struct semaphore), GFP_KERNEL);
    if (!new_group_dev->wq_sem)
    {
        kmalloc_err("group_dev->wq_sem");
        goto wq_sem_fail;
    }
    sema_init(new_group_dev->wq_sem, 1);
    dbg("new_group_dev->wq_sem allocated\n");

    /* Allocate and initialize delayed work list. */
    new_group_dev->delay_list = kmalloc(sizeof(struct list_head), GFP_KERNEL);
    if (!new_group_dev->delay_list)
    {
        kmalloc_err("group_dev->pending_list");
        goto delayed_list_fail;
    }
    INIT_LIST_HEAD(new_group_dev->delay_list);
    dbg("new_group_dev->delay_list allocated\n");

    /* Initialize list member. */
    INIT_LIST_HEAD(&new_group_dev->list);
    dbg("new_group_dev->list initialized\n");

    /* Initialize wait queue. */
    init_waitqueue_head(&new_group_dev->wait_queue);
    dbg("new_group_dev->wait_queue initialized\n");

    /* Initialize workqueue. */
    init_workqueue(new_group_dev);
    dbg("new_group_dev->wait_queue initialized\n");

    /* If no major has been already initialized, then this is the first time. Hence,
       dynamically allocate region for character devices. System will provide 
       major. */
    if (!major)
    {
        err = alloc_chrdev_region(&dev, 0, GROUP_DEV_COUNT, GROUP_DEVICE_NAME);
        major = MAJOR(dev);
        group_devs->major = major;
        dbg("alloc_chrdev_region for major %d and minor %d\n", major, minor);
    }

    /* Registration failed. */
    if (err < 0)
    {
        err("registration failed for major %d\n", major);
        goto dev_reg_fail;
    }

    /* Create the class group devices belong to. */
    if (!group_dev_class)
    {
        group_dev_class = class_create(THIS_MODULE, GROUP_CLASS_NAME);
        dbg("group_dev_class initialized\n");
    }

    /* Initialize char dev structure.
       Associate specific file operations to the group device.
       Create the group device. */
    cdev_init(&new_group_dev->cdev, &group_dev_fops);
    cdev_add(&new_group_dev->cdev, MKDEV(major, minor), 1);
    device_create(group_dev_class, NULL, MKDEV(major, minor), NULL, device_name);
    new_group_dev->minor = minor;

    info("%s with major %d and minor %d created\n", device_name, major, minor);
    kfree(device_name); /* Name not needed anymore. */
    goto exit;

    /* Each fail should "abort" previous successful operations. */
dev_reg_fail:
    kfree(new_group_dev->delay_list);
    dbg("dev_reg_fail\n");
delayed_list_fail:
    kfree(new_group_dev->wq_sem);
    dbg("delayed_list_fail\n");
wq_sem_fail:
    kfree(new_group_dev->pending_sem);
    dbg("wq_sem_fail\n");
pnd_sem_fail:
    kfree(new_group_dev->pending_list);
    dbg("pnd_sem_fail\n");
pnd_list_fail:
    kfree(new_group_dev->message_sem);
    dbg("pnd_list_fail\n");
msg_sem_fail:
    kfree(new_group_dev->message_list);
    dbg("msg_sem_fail\n");
msg_list_fail:
    kfree(new_group_dev);
    dbg("msg_list_fail\n");
dev_alloc_fail:
    kfree(device_name);
    dbg("dev_alloc_fail\n");
dev_name_fail:
    new_group_dev = NULL;
    dbg("dev_name_fail\n");
exit:
    dbg_end();
    return new_group_dev;
}

int install_group(struct group_t *group_desc)
{
    int ret;
    unsigned char desc;
    struct group_dev *gd;

    dbg_start();
    ret = -1;

    if (!group_desc)
    {
        ref_err("group_desc");
        ret = -1;
        goto exit;
    }

    desc = group_desc->desc;

    /* Check number of installed group devices. */
    if (desc >= GROUP_DEV_COUNT)
    {
        warn("desc %d > %d GROUP_DEV_COUNT\n", desc, GROUP_DEV_COUNT);
        goto exit;
    }

    /* Initialize group_devs. */
    if (!group_devs)
    {
        init_group_devs();
        dbg("group_devs initialized\n");
    }

    /* Seek for a group device matching the descriptor. */
    gd = get_group(desc);
    if (gd) /* Seek was successful. */
    {
        ret = 0;
        goto exit;
    }

    /* Check if there is space for another group device. */
    if (GROUP_DEV_COUNT < group_devs->used + 1)
    {
        warn("cannot install additional group devices\n");
        goto exit;
    }

    /* Seek was non successful and there is enough space.
       Install the group device. */
    gd = _install_group(desc);
    if (gd)
    {
        dbg("obtained group_dev for desc %d\n", desc);

        /* Add group device to overall list. */
        list_add(&gd->list, group_devs->group_devs_list);
        dbg("added group_dev to the list\n");

        /* Increment number of used group devices. */
        group_devs->used++;

        group_devs_list_print(group_devs->group_devs_list);
        ret = 0;
        goto exit;
    }

exit:
    dbg_end();
    return ret;
}

void group_free(struct group_dev *dev)
{
    struct message *tmp_msg;
    struct list_head *pos, *q;

    dbg_start();
    info("freeing group_dev%d\n", dev->minor);

    /* If the barrier has been raised, destroy it
       and wake up waiting threads. */
    if (is_barrier_up(dev)) //waitqueue_active(&dev->wait_queue)
    {
        clear_barrier(dev);
        dbg("clear_barrier\n");
        wake_up_all(&dev->wait_queue);
        dbg("wake_up_all\n");
    }

    /* Free workqueue before flushing it such that
       delayed messages become available. */
    if (dev->delay_wq)
    {
        fflush_workqueue(dev);
        dbg("fflush_workqueue\n");
        destroy_workqueue(dev->delay_wq);
        dbg("destroy_workqueue\n");
    }

    /* Free workqueue semaphore. */
    if (dev->wq_sem)
    {
        kfree(dev->wq_sem);
        dbg("kfreed wq_sem\n");
    }

    /* Free delay_list. Should be empty. */
    if (dev->delay_list)
    {

        if (!list_empty(dev->delay_list))
        {
            warn("delay_list not emptied\n");
        }

        /* Finally, free the list. */
        kfree(dev->delay_list);
        dbg("kfreed dev->delay_list\n");
    }

    /* Free pending semaphore. */
    if (dev->pending_sem)
    {
        kfree(dev->pending_sem);
        dbg("kfreed pending_sem\n");
    }

    /* Free pending list. Should be empty. */
    if (dev->pending_list)
    {
        if (!list_empty(dev->pending_list))
        {
            /* Free messages if any. */
            list_for_each_prev_safe(pos, q, dev->pending_list)
            {
                dbg("traversing dev->pending_list\n");
                tmp_msg = list_entry(dev->pending_list, struct message, list);
                if (tmp_msg)
                {
                    list_del(&tmp_msg->list);
                    if (tmp_msg->data)
                    {
                        dbg("kfree tmp_msg->data\n");
                        kfree(tmp_msg->data);
                    }
                    dbg("kfree tmp_msg\n");
                    kfree(tmp_msg);
                }
                else
                {
                    ref_err("tmp_msg");
                    break;
                }
            }
        }

        if (!list_empty(dev->pending_list))
        {
            warn("pending_list not emptied\n");
        }

        /* Finally, free the list. */
        kfree(dev->pending_list);
        dbg("kfreed dev->pending_list\n");
    }

    /* Free message semaphore. */
    if (dev->message_sem)
    {
        kfree(dev->message_sem);
        dbg("kfreed dev->message_sem\n");
    }

    /* Free message list. */
    if (dev->message_list)
    {
        message_list_print(dev->message_list);
        if (!list_empty(dev->message_list))
        {
            /* Free messages if any. */
            list_for_each_prev_safe(pos, q, dev->message_list)
            {
                dbg("traversing dev->message_list\n");
                tmp_msg = list_entry(pos, struct message, list);
                if (tmp_msg)
                {
                    list_del(&tmp_msg->list);
                    if (tmp_msg->data)
                    {
                        message_print(tmp_msg);
                        dbg("kfree tmp_msg->data\n");
                        kfree(tmp_msg->data);
                    }
                    dbg("kfree tmp_msg\n");
                    kfree(tmp_msg);
                }
                else
                {
                    ref_err("tmp_msg");
                    break;
                }
            }
        }

        if (!list_empty(dev->message_list))
        {
            warn("message_list not emptied\n");
        }

        /* Finally, free the list. */
        kfree(dev->message_list);
        dbg("kfreed dev->message_list\n");
    }

    /* End of the story, free the group device managing structure. */
    kfree(dev);

    dbg_end();
    return;
}

void group_free_all(void)
{
    int minor, major, to_free;
    struct group_dev *tmp_dev;
    struct list_head *pos, *q;

    dbg_start();

    /* Check group devices structure. */
    if (!group_devs)
    {
        ref_err("group_devs");
        to_free = 0;
        goto exit;
    }

    to_free = group_devs->used;
    major = group_devs->major;

    /* Check group devices list. */
    if (!group_devs->group_devs_list)
    {
        ref_err("group_devs_list");
        goto exit;
    }

    group_devs_list_print(group_devs->group_devs_list);
    dbg("to_free = %d", to_free);

    if (!list_empty(group_devs->group_devs_list))
    {
        /* Traversing all group devices. */
        list_for_each_prev_safe(pos, q, group_devs->group_devs_list)
        {
            tmp_dev = list_entry(pos, struct group_dev, list);

            if (!tmp_dev)
            {
                ref_err("tmp_dev");
                break;
            }

            minor = tmp_dev->minor;

            list_del(&tmp_dev->list); /* Delete from list. */
            dbg("group_dev%d list_del\n", minor);

            device_destroy(group_dev_class, MKDEV(major, minor)); /* Destroy device */
            dbg("group_dev%d device_destroy\n", minor);

            cdev_del(&tmp_dev->cdev); /* Delete char dev structure. */
            dbg("group_dev%d cdev_del\n", minor);

            group_free(tmp_dev); /* Free the structure matching minor. */
            dbg("group_dev%d structure freed\n", minor);

            to_free--; /* Decrease for further check.*/
        }
    }

    /* Unregister character device region. */
    unregister_chrdev_region(MKDEV(major, 0), GROUP_DEV_COUNT);
    dbg("unregister_chrdev_region major %d - minor %d\n", major, 0);

    class_destroy(group_dev_class); /* Destroy the class group devices belong to. */
    dbg("group_class_destroy\n");

    /* Free group devices list and structure. */
    kfree(group_devs->group_devs_list);
    kfree(group_devs);

exit:
    /* The cleanup should free n = used group devices. If to_free
       is not 0, some group device has survived. */
    info("after cleanup %d group_dev survived\n", to_free);
    dbg_end();
    return;
}
