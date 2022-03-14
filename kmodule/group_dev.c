#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>

#include "../common.h"
#include "kern.h"
#include "ioctl.h"
#include "group_dev.h"

/* Associate specialized file operations. */
struct file_operations group_dev_fops = {
    .owner = THIS_MODULE,
    .open = group_open,
    .release = group_release,
    .read = group_read,
    .write = group_write,
    .unlocked_ioctl = group_unlocked_ioctl,
    .flush = group_flush};

void message_print(struct message *msg)
{
    dbg_start();

    if (!msg)
    {
        dbg("0 - ''\n");
        return;
    }

    if (!msg->data)
    {
        dbg("%ld - ''\n", msg->data_size);
        return;
    }

    dbg("%ld - '%s'\n", msg->data_size, msg->data);

    dbg_end();
    return;
}

void message_list_print(struct list_head *list)
{
    int i;
    struct message *tmp;
    struct list_head *pos;

    dbg_start();

    /* Check for list. */
    if (!list)
    {
        ref_err("list");
        goto exit;
    }

    /* If list is empty, nothing to do. */
    if (list_empty(list))
    {
        dbg("message_list is empty\n");
        goto exit;
    }

    i = 0;
    list_for_each_prev(pos, list)
    {
        tmp = list_entry(pos, struct message, list);
        dbg("%d, ", i++);
        message_print(tmp);
    }

exit:
    dbg_end();
    return;
}

void _set_delay(struct group_dev *dev, long delay)
{
    long jiffies;

    dbg_start();

    jiffies = msecs_to_jiffies(delay); /* msec -> jiffies. */
    dbg("msecs %ld -> %ld jiffies\n", delay, jiffies);
    dev->delay = jiffies;
    dbg("group_dev%d delay set to %ld (%ld)\n", dev->minor, delay, dev->delay);

    dbg_end();
    return;
}

long get_delay_msecs(struct group_dev *dev)
{
    dbg_start();
    dbg_end();
    return jiffies_to_msecs(dev->delay);
}

long get_delay_jiffies(struct group_dev *dev)
{
    dbg_start();
    dbg_end();
    return dev->delay;
}

int is_flushing(struct group_dev *dev)
{
    dbg_start();
    dbg("FLUSHING_BIT is %d\n", check_bit(dev->flags, FLUSHING_BIT));
    dbg_end();
    return check_bit(dev->flags, FLUSHING_BIT);
}

void set_flushing(struct group_dev *dev)
{
    dbg_start();
    set_bit(dev->flags, FLUSHING_BIT); /* Flushing set up. */
    dbg_end();
    return;
}

void clear_flushing(struct group_dev *dev)
{
    dbg_start();
    clear_bit(dev->flags, FLUSHING_BIT); /* Flushing cleared. */
    dbg_end();
    return;
}

int is_barrier_up(struct group_dev *dev)
{
    dbg_start();
    dbg("BARRIER_BIT is %d\n", check_bit(dev->flags, BARRIER_BIT));
    dbg_end();
    return check_bit(dev->flags, BARRIER_BIT);
}

void set_barrier(struct group_dev *dev)
{
    dbg_start();
    set_bit(dev->flags, BARRIER_BIT); /* Barrier set up. */
    dbg_end();
    return;
}

void clear_barrier(struct group_dev *dev)
{
    dbg_start();
    clear_bit(dev->flags, BARRIER_BIT); /* Barrier cleared. */
    dbg_end();
    return;
}

int init_workqueue(struct group_dev *dev)
{
    /* Create singlethrad workqueue. */
    dev->delay_wq = create_singlethread_workqueue(DELAYED_WORKQUEUE_NAME);
    dbg("group_dev%d create_st_workqueue\n", dev->minor);
    return dev->delay_wq ? 0 : -1;
}

void _fflush_workqueue(struct group_dev *dev)
{
    dbg_start();

    /* Check for items to be not NULL. */
    if (!dev->pending_sem)
    {
        ref_err("pending_sem");
        goto exit;
    }

    if (!dev->message_sem)
    {
        ref_err("message_sem");
        goto exit;
    }

    if (!dev->pending_list)
    {
        ref_err("pending_list");
        goto exit;
    }

    if (!dev->message_list)
    {
        ref_err("message_list");
        goto exit;
    }

    down(dev->pending_sem); /* Acquire resource. */
    down(dev->message_sem); /* Acquire resource. */

    /* Join pending_list to message_list. */
    /*
       Before join:
        pending_list := A-B-C
        message_list := D-E-F
       After join:
        pending_list := empty
        message_list := A-B-C-D-E-F
    */
    list_splice_tail_init(dev->pending_list, dev->message_list);
    dbg("pending_list joined to message_list\n");
    up(dev->pending_sem); /* Release resource. */
    up(dev->message_sem); /* Release resource. */

exit:
    dbg_end();
    return;
}

void fflush_workqueue(struct group_dev *dev)
{
    struct work *work;
    struct list_head *pos, *q;
    dbg_start();

    /* Check dev. */
    if (!dev)
    {
        ref_err("dev");
        goto exit;
    }

    /* First, move all pending messages. */
    _fflush_workqueue(dev);

    /* Check for items to be not NULL. */
    if (!dev->delay_wq)
    {
        ref_err("delay_wq");
        goto exit;
    }

    if (!dev->delay_list)
    {
        ref_err("delay_list");
        goto exit;
    }

    if (!dev->wq_sem)
    {
        ref_err("wq_sem");
        goto exit;
    }

    /* All delayed work is going to be executed. */
    down(dev->wq_sem); /* Acquire resource. */

    set_flushing(dev); /* Set flushing flag. */

    /* pending_list is empty => no work to be flushed. */
    if (list_empty(dev->delay_list))
    {
        dbg("delay_list empty");
        goto dl_exit;
    }

    /* Traverse safely delay_list. */
    list_for_each_prev_safe(pos, q, dev->delay_list)
    {
        work = list_entry(pos, struct work, list); /* Retrieve work. */
        if (work)
        {
            dbg("work retrieved\n");
            list_del(&work->list); /* Delete it from list. */
            dbg("work list_del\n");
            flush_delayed_work(&work->dwork); /* Instantly execute it. */
            dbg("flush_delayed_work\n");
        }
        else /* Stop traversal. Crashes are in ambush. */
        {
            dbg("no work retrieved\n");
            goto dl_exit;
        }
    }

dl_exit:
    drain_workqueue(dev->delay_wq); /* Drain the workqueue. */
    clear_flushing(dev);            /* Reset flushing flag. */
    up(dev->wq_sem);                /* Release resource. */
exit:
    dbg_end();
    return;
}

void delayed_work_fun(struct work_struct *work)
{
    struct group_dev *dev;
    struct message *msg;
    struct delayed_work *dwork_p;
    struct work *dwork;

    dbg_start();

    /* Check work. */
    if (!work)
    {
        ref_err("work");
        goto exit;
    }

    /* Need to convert work to delayed work and to 
       retrieve the specific structure. */
    dwork_p = to_delayed_work(work);
    dwork = container_of(dwork_p, struct work, dwork);

    if (!dwork)
    {
        ref_err("dwork");
        goto exit;
    }

    dev = dwork->dev;
    if (!dev)
    {
        ref_err("dev");
        goto exit;
    }

    if (!dev->wq_sem)
    {
        ref_err("wq_sem");
        goto exit;
    }

    /* Need to check all semaphores and lists to be not null. */
    if (!dev->pending_sem)
    {
        ref_err("dev->pending_sem");
        goto exit;
    }

    if (!dev->pending_list)
    {
        ref_err("dev->pending_list");
        goto exit;
    }

    if (!dev->message_sem)
    {
        ref_err("dev->message_sem");
        goto exit;
    }

    if (!dev->message_list)
    {
        ref_err("dev->message_list");
        goto exit;
    }

    down(dev->pending_sem); /* Acquire resource. */

    /* No message to move. */
    if (list_empty(dev->pending_list))
    {
        dbg("pending_list empty\n");
        goto pnd_exit;
    }

    /* Retrieve the last pending message (according to the FIFO
       policy) and remove it from the pending list. */
    msg = list_last_entry(dev->pending_list, struct message, list);

    /* Check if any message was retrieved. */
    if (!msg)
    {
        dbg("no message retrieved\n");
        goto pnd_exit;
    }

    /* Remove message from pending list. */
    list_del(&msg->list);

    up(dev->pending_sem);   /* Release resource. */
    down(dev->message_sem); /* Acquire resource. */

    /* Add message to the message list. */
    list_add(&msg->list, dev->message_list);
    up(dev->message_sem); /* Release resource. */

    info("moved %ld: '%s' after %lu msecs\n", msg->data_size, msg->data, get_delay_msecs(dev));
    goto exit;

pnd_exit:
    up(dev->pending_sem); /* Release resource. */
exit:
    /* Free delayed work. */
    if (!is_flushing(dev))
    {
        /* When FLUSHING_BIT is set, list management is left
           to the fflush_workqueue() function. In other terms,
           the FLUSHING_BIT discriminates whether the delayed
           work is being executed because timer expired or if
           the workqueue is being flushed. Required for efficient
           list management without crashes. */
        dbg("not flushing\n");
        list_del(&dwork->list);
        dbg("dwork list_del\n");
    }
    kfree(dwork);
    dbg("kfreed dwork\n");
    dbg_end();
    return;
}

void add_delayed_work(struct group_dev *dev)
{
    struct work *work;

    dbg_start();

    /* Check workqueue. */
    if (!dev->delay_wq)
    {
        ref_err("delay_wq");
        goto exit;
    }

    /* Allocate delayed work structure. */
    work = kmalloc(sizeof(struct work), GFP_KERNEL);
    if (!work)
    {
        kmalloc_err("work");
        goto exit;
    }

    /* Initialize delayed work. */
    INIT_DELAYED_WORK(&work->dwork, delayed_work_fun);
    dbg("delayed_work initialized\n");
    work->dev = dev;
    INIT_LIST_HEAD(&work->list);

    down(dev->wq_sem); /* Acquire resource. */

    /* Queue the delayed work into the workqueue. */
    queue_delayed_work(dev->delay_wq, &work->dwork, dev->delay);
    /* Add the work to the delayed work list */
    list_add(&work->list, dev->delay_list);

    up(dev->wq_sem); /* Release resource. */
    dbg("delayed_work queued\n");

exit:
    dbg_end();
    return;
}

int group_open(struct inode *inode, struct file *filp)
{
    struct group_dev *dev;

    dbg_start();

    dev = container_of(inode->i_cdev, struct group_dev, cdev);
    /* Check for device structure. */
    if (!dev)
    {
        ref_err("dev");
        dbg_end();
        return -1;
    }
    /* Associate the group device structure to inode private data. */
    filp->private_data = dev;

    dbg_end();
    return 0;
}

int group_release(struct inode *inode, struct file *filp)
{
    dbg_start();
    dbg_end();
    return 0;
}

ssize_t group_read(struct file *filp, char __user *buf, size_t length, loff_t *offset)
{
    ssize_t ret;
    struct message *msg;
    struct group_dev *dev;

    dbg_start();
    ret = -1;

    /* Check for filp. */
    if (!filp)
    {
        ref_err("filp");
        goto exit;
    }

    dev = filp->private_data;
    /* Check for device structure. */
    if (!dev)
    {
        ref_err("dev");
        goto exit;
    }

    down(dev->message_sem); /* Acquire resource. */

    /* Check for group device message list. */
    if (!dev->message_list)
    {
        ref_err("list");
        goto msg_sem_exit;
    }

    /* Check if group device message list is empty. */
    if (list_empty(dev->message_list))
    {
        dbg("message_list empty\n");
        ret = 0;
        goto msg_sem_exit;
    }

    /* Retrieve message according to FIFO policy. */
    msg = list_last_entry(dev->message_list, struct message, list);
    if (!msg)
    {
        ref_err("msg");
        goto msg_sem_exit;
    }

    dbg("list has a message to be retrieved\n");
    list_del(&msg->list);   /* Remove message from message list. */
    dev->messages_number--; /* Decrease number of messages in the device. */

    up(dev->message_sem); /* Release resource. */

    /* Tailor length to actual data size. In particular:
       if length > data_size,   send data_size bytes;
       otherwise,               send length bytes. */
    if (length > msg->data_size)
    {
        length = msg->data_size;
    }

    /* Send data to userspace. */
    if (copy_to_user(buf, msg->data, length))
    {
        err("copy_to_user error\n");
        goto exit;
    }
    info("copy_to_user %ld bytes '%s'\n", length, msg->data);

    /* Free the message and its data. */
    kfree(msg->data);
    kfree(msg);
    ret = length;
    goto exit;

msg_sem_exit:
    up(dev->message_sem); /* Release resource. */
    dbg_cs_end();
exit:
    dbg_end();
    return ret;
}

ssize_t group_write(struct file *filp, const char *buf, size_t length, loff_t *offset)
{
    char *data;
    ssize_t ret;
    struct message *msg;
    struct group_dev *dev;

    dbg_start();
    ret = -1;

    /* Check for filp. */
    if (!filp)
    {
        ref_err("filp");
        goto exit;
    }

    dev = filp->private_data;
    /* Check for group device structure. */
    if (!dev)
    {
        ref_err("dev");
        goto exit;
    }

    if (length <= 0) {
        err("length not valid\n");
        goto exit;
    }

    if (length > max_message_size) {
        length = max_message_size;
    }

    /* Allocate data to be added to the group device. */
    data = kzalloc(length * sizeof(char), GFP_KERNEL);
    if (!data)
    {
        kzalloc_err("data");
        /* First fail, just return error. */
        goto exit;
    }
    dbg("data allocated\n");

    msg = kzalloc(sizeof(struct message), GFP_KERNEL);
    if (!msg)
    {
        kzalloc_err("message");
        /* Second fail, free previous. */
        goto data_fail;
    }
    dbg("msg allocated\n");

    down(dev->message_sem); /* Acquire resource. */

    /* Check if there is space to host messages. */
    if (dev->messages_number >= max_storage_size)
    {
        warn("no space to write\n");
        /* Third fail, must release resource. */
        ret = 0;
        goto msg_fail;
    }

    /* Check for group device message list. */
    if (!dev->message_list)
    {
        ref_err("message_list");
        /* Fourth fail, must release resource. */
        goto msg_fail;
    }

    /* Get data from userspace. */
    if (copy_from_user(data, buf, length))
    {
        err("copy_from_user %ld bytes\n", length);
        /* Fifth fail, must release resource. */
        goto msg_fail;
    }
    dbg("copy_from_user %ld bytes ", length);

    if (!data)
    {
        ref_err("\ndata");
        /* Sixth fail, must release resource. */
        goto msg_fail;
    }

    /* Apply the terminator character.
       Could be removed due to kzalloc. */
    data[length] = 0;
    dbg("'%s'\n", data);

    /* Initialize message with actual data. */
    msg->data = data;
    msg->data_size = length;
    dev->messages_number++; /* Increase number of stored messages. */
    info("group_dev%d contains %d messages\n", dev->minor, dev->messages_number);
    /* If a delay was set, add message to pending list 
       and queue a work in the workqueue. */
    if (dev->delay)
    {
        dbg("group_dev%d has a delay of %ld msecs\n", dev->minor, get_delay_msecs(dev));
        up(dev->message_sem);                    /* Release resource. */
        down(dev->pending_sem);                  /* Acquire resource. */
        list_add(&msg->list, dev->pending_list); /* Add message to pending list. */
        up(dev->pending_sem);                    /* Release resource. */
        add_delayed_work(dev);                   /* Initalize and queue delayed work. */
        dbg("delayed work queued\n");
    }
    /* Otherwise, add message to message list. */
    else
    {
        dbg("group_dev%d has no delay", dev->minor);
        list_add(&msg->list, dev->message_list); /* Add message to message list. */
        up(dev->message_sem);                    /* Release resource. */
        message_list_print(dev->message_list);
    }

    info("written %ld bytes '%s' with delay %ld msecs\n", length, data, get_delay_msecs(dev));
    ret = length;
    goto exit;

/*  Each fail will return -1.
    Second fail, must just free data.
    All other fail must free message and relese the resource, since
    they all sit in the critical section. */
msg_fail:
    up(dev->message_sem); /* Release resource. */
    kfree(msg);
data_fail:
    kfree(data);
exit:
    dbg_end();
    return ret;
}

long group_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret;
    struct group_dev *dev;

    dbg_start();
    ret = -1;

    /* Check for filp. */
    if (!filp)
    {
        ref_err("filp");
        goto exit;
    }

    dev = filp->private_data;
    /* Check for group device structure. */
    if (!dev)
    {
        ref_err("dev");
        goto exit;
    }

    /* IOCTL cases: */
    /* First case,  a thread wants to sleep. 
       Second case, a thread wants to awake the whole barrier.
       Third case,  a thread wants to set a delay. 
       Fourth case, a thread wants to immediatly publish delayed messages. */
    switch (cmd)
    {
    case IOCTL_SLEEP_ON_BARRIER:
        info("IOCTL_SLEEP_ON_BARRIER\n");
        set_barrier(dev); /* Set the barrier up. */
        /* Add thread to wait queue until barrier is destroyed. */
        wait_event(dev->wait_queue, !is_barrier_up(dev));
        ret = 0;
        goto exit;
    case IOCTL_AWAKE_BARRIER:
        info("IOCTL_AWAKE_BARRIER\n");
        /* Check if barrier is up and the wait queue is not empty. */
        if (is_barrier_up(dev))
        {
            clear_barrier(dev); /* Destroy the barrier. */
            dbg("clear_barrier\n");
        }
        if (waitqueue_active(&dev->wait_queue))
        {
            /* Wake up all threads in the device wait queue. Since barrier
               was previously destroyed, every thread awakes up. */
            wake_up_all(&dev->wait_queue);
            dbg("wake_up_all\n");
        }
        ret = 0;
        goto exit;
    case IOCTL_SET_SEND_DELAY:
        info("IOCTL_SET_SEND_DELAY\n");
        if ((long) arg < 0) {
            err("negative delay\n");
            goto exit;
        }
        _set_delay(dev, (long) arg); /* Set delay. */
        ret = 0;
        goto exit;
    case IOCTL_REVOKE_DELAYED_MESSAGES:
        info("IOCTL_REVOKE_DELAYED_MESSAGES\n");
        /* Flush the workqueue. */
        fflush_workqueue(dev);
        ret = 0;
        goto exit;
    }

exit:
    dbg_end();
    return ret;
}

int group_flush(struct file *filp, fl_owner_t id)
{
    dbg_start();
    dbg_end();
    return 0;
}
