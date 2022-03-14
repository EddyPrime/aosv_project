#pragma once

#include <linux/fs.h>
#include <linux/list.h>
#include <linux/cdev.h>
#include <linux/workqueue.h>
#include <linux/wait.h>

/**
 * Macros for correctly and easily managing bitwise operations. 
 * Instead of having a variable for each flag, it is used a
 * variable where each bit is related to a flag. 
 */

#define set_bit(var, n) var |= 1UL << n
#define clear_bit(var, n) var &= ~(1UL << n)
#define toggle_bit(var, n) var ^= 1UL << n
#define check_bit(var, n) (var >> n) & 1U

#define BARRIER_BIT 0
#define FLUSHING_BIT 1

/**
 * Name of the workqueue for delayed work. 
 */

#define DELAYED_WORKQUEUE_NAME "delayed_workqueue"

/**
 * Retrieve the two parameters from outside.
 */

extern unsigned int max_message_size;
extern unsigned int max_storage_size;

/**
 * struct work - struct for delayed work.
 * 
 * @dwork: delayed work struct
 * @dev: pointer to the associated group_dev
 * @list: field required to include works into lists
 * 
 * This struct manages the publishing of messages belonging
 * to group devices whose delay is strictly greater than 0.
 */
struct work
{
    struct delayed_work dwork;
    struct group_dev *dev;
    struct list_head list;
};

/**
 * struct message - struct for messages.
 * 
 * @data_size: the length of the message
 * @data: the text message
 * @list: field required to include messages into lists
 * 
 * This struct represents messages exchanged among processes
 * and threads.
 */
struct message
{
    size_t data_size;
    char *data;
    struct list_head list;
};

/**
 * struct group_dev - struct for each group device.
 * 
 * @cdev: kernel struct that represents a char device
 * @minor: the minor number associated to the device
 * @flags: variable containing flags
 * 
 * @messages_number: the number of messages currently stored
 * into the group device
 * @message_sem: semaphore protecting the list of messages
 * @message_list: list containing all publishedmessages of 
 * the group device
 * 
 * @delay: jiffies of delay for the publication of messages
 * @wq_sem: semaphore protecting the workqueue
 * @delay_wq: the workqueue of delayed works
 * @delay_list: the list of delayed works
 * 
 * @pending_sem: semaphore protecting the pending list
 * @pending_list: list of delayed messages
 * 
 * @wait_queue: list containing all threads put into wait
 * after sleeping on the barrier of this group device
 * 
 * @list: field required to include group devices into lists
 * 
 * This struct represents the group device.
 */
struct group_dev
{
    struct cdev cdev;
    unsigned char minor;
    char flags;

    unsigned int messages_number;
    struct semaphore *message_sem;
    struct list_head *message_list;

    unsigned long delay;
    struct semaphore *wq_sem;
    struct workqueue_struct *delay_wq;
    struct list_head *delay_list;

    struct semaphore *pending_sem;
    struct list_head *pending_list;

    wait_queue_head_t wait_queue;

    struct list_head list;
};

extern struct file_operations group_dev_fops;

int group_open(struct inode *inode, struct file *filp);
int group_release(struct inode *inode, struct file *filp);
ssize_t group_read(struct file *filp, char *buff, size_t length, loff_t *offset);
ssize_t group_write(struct file *filp, const char *buff, size_t length, loff_t *offset);
long group_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int group_flush(struct file *filp, fl_owner_t id);

/**
 * message_print() - prints a message.
 * 
 * @msg: the message to be printed
 * 
 * Auxiliary function which prints a message.
 * 
 * Returns:
 * void
 */
void message_print(struct message *msg);

/**
 * message_list_print() - prints all messages of a group 
 * device.
 * 
 * @list: the head of the messages list
 * 
 * Auxiliary function which prints all messages stored in
 * a list. May be used both for pending and message lists.
 * 
 * Returns:
 * void
 */
void message_list_print(struct list_head *list);

/**
 * _set_delay() - sets group device delay.
 * 
 * @dev: the group device structure
 * @delay: the delay to be set
 * 
 * Sets @dev's delay to @delay.
 * 
 * Returns:
 * void
 */
void _set_delay(struct group_dev *dev, long delay);

/**
 * get_delay_msecs() - gets group device delay.
 * 
 * @dev: the group device structure
 * 
 * Retrieves the value in milliseconds of the delay set
 * for @dev.
 * 
 * Returns:
 * long - delay
 */
long get_delay_msecs(struct group_dev *dev);

/**
 * get_delay_jiffies() - gets group device delay.
 * 
 * @dev: the group device structure
 * 
 * Retrieves the value in jiffies of the delay set
 * for @dev.
 * 
 * Returns:
 * long - delay
 */
long get_delay_jiffies(struct group_dev *dev);

/**
 * is_flushing() - checks whether the flushing bit is set.
 * 
 * @dev: the specific group device
 *  
 * Retrieves information on whether @dev's flushing bit is 
 * set or not.
 * 
 * Returns:
 * 1 - flushing is set for @dev
 * 0 - flushing is not set for @dev
 */
int is_flushing(struct group_dev *dev);

/**
 * set_flushing() - sets the flushing bit.
 * 
 * @dev: the specific group device
 * 
 * The flushing bit of @dev's flags variable is set. This
 * helps managing delayed works flushing.
 * 
 * Returns:
 * void
 */
void set_flushing(struct group_dev *dev);

/**
 * clear_flushing() - clears the flushing bit.
 * 
 * @dev: the specific group device
 * 
 * The flushing bit of @dev's flags variable is reset.
 * 
 * Returns:
 * void
 */
void clear_flushing(struct group_dev *dev);

/**
 * is_barrier_up() - checks if barriers was raised for a 
 * specific group device.
 * 
 * @dev: the specific group device
 *  
 * Retrieves information on whether @dev's barrier 
 * is raised up or not.
 * 
 * Returns:
 * 1 - barrier is raised for @dev
 * 0 - barrier is not raised for @dev
 */
int is_barrier_up(struct group_dev *dev);

/**
 * set_barrier() - raises barrier for specifc group device.
 * 
 * @dev: the specific group device
 * 
 * Raises @dev's barrier. It means that some thread is sleeping
 * on dev. As long as the barrier is up, sleeping threads
 * cannot be awakened.
 * 
 * Returns:
 * void
 */
void set_barrier(struct group_dev *dev);

/**
 * clear_barrier() - destroys barrier for specifc group device.
 * 
 * @dev: the specific group device
 * 
 * Destroys @dev's barrier. It means that some thread is willing
 * to awake threads sleeping on dev. In order for threads to be
 * awakened, the barried MUST be destroyed.
 * 
 * Returns:
 * void
 */
void clear_barrier(struct group_dev *dev);

/**
 * init_workqueue() - inits a delayed workqueue.
 * 
 * @dev: the group device whose workqueue must be initialized
 * 
 * Initializes @dev's workqueue. This structure is managing the
 * publication of messages when a delay is set for @dev. Indeed,
 * if a delay is set, messages are unavailable from the time
 * they are written into the group device until the delay 
 * expires. 
 * 
 * Returns:
 * 0 - ok
 * -1 - ko
 */
int init_workqueue(struct group_dev *dev);

/**
 * _fflush_workqueue() - flushes a workqueue.
 * 
 * @delay_wq: the workqueue to be flushed
 * 
 * Flushes @dev pending messages. The pending list is joined
 * (becoming the new head) to the message list. In other 
 * terms, all messages whose publication was delayed are 
 * made available.
 * Sempahore protected.
 * 
 * Returns:
 * void
 */
void _fflush_workqueue(struct group_dev *dev);

/**
 * fflush_workqueue() - flushes a workqueue.
 * 
 * @delay_wq: the workqueue to be flushed
 * 
 * Wrapper for _fflush_workqueue(). In addition, all pending 
 * work in @dev's @delay_wq is executed thanks to @delay_list.
 * 
 * Returns:
 * void
 */
void fflush_workqueue(struct group_dev *dev);

/**
 * delayed_work_fun() - delayed works function.
 * 
 * @work: struct required to run delayed work
 * 
 * This is the function that will take care of the publication
 * itself of delayed messages. It boils down to move  messages 
 * from a list to another. When a work in the workqueue is 
 * executed, this function is invoked. The message is moved 
 * from the pending list to the message list.
 * 
 * Returns:
 * void
 */
void delayed_work_fun(struct work_struct *work);

/**
 * add_delayed_work() - adds delayed works.
 * 
 * @work: struct required to run delayed work
 * 
 * Tries to add a work in @dev's workqueue. This function is
 * invoked when a thread is writing a message on a group
 * device whose delay is set (to something different from 0).
 * The work is queued in the workqueue and will be executed
 * as its specific delay expires. Moreover, the work is added
 * to @dev's delayed works list for flushing purposes.
 * 
 * Returns:
 * void
 */
void add_delayed_work(struct group_dev *dev);