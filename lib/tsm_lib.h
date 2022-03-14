#include <string.h>
#include <sys/syscall.h>

#include "../common.h"

#define TSM_DEV "/dev/tsm"
#define GROUP_DEV "/dev/synch/group_dev%d"
#define GROUP_DEV_LENGTH strlen(GROUP_DEV) + 3

#define SLEEP_TIME 0.0000001
#define ATTEMPTS 10000

#ifndef DEBUG
#define DEBUG 0
#endif

/** 
 * gettid() - retrieves thread identifier.
 * 
 * Returns:
 * long - thread identifier.
 */
#define gettid(void) syscall(__NR_gettid)

#define info(format, ...) printf(format "\n", ##__VA_ARGS__)
#define err(format, ...) info("ERR: " format, ##__VA_ARGS__)

#define dbg(format, ...) \
    if (DEBUG)           \
    info(format, ##__VA_ARGS__)

#define start(name) info("EXECUTING %s\n", name)
#define end() info(DONE_MSG)

#define tid_info(format, ...) info("[%ld] " format, gettid(), ##__VA_ARGS__)
#define tid_err(format, ...) tid_info("ERR: " format, ##__VA_ARGS__)

#define tid_start() tid_info(START_MSG)
#define tid_end() tid_info(DONE_MSG)

/**
 * max_message_size - upperbound to the size of each
 * single message.
 */
static unsigned int max_message_size = 0;

/**
 * open_group() - opens a group device.
 * 
 * @group_descriptor: the descriptor of the group device
 * 
 * Opens a group. The provided descriptor will be used to open,
 * or to install if it does not exist, a group device.
 * 
 * Returns:
 * < 0  - error
 * >= 0 - file descriptor for group device
 */
int open_group(struct group_t *group_descriptor);

/**
 * send_message() - writes a message to the group device.
 * 
 * @fd: the file descriptor
 * @msg: the message to be sent
 * 
 * Writes @length bytes of the message stored in @msg to
 * the group device related to the file descriptor @fd.
 * 
 * Returns:
 * -1   - error
 * >= 0 - number of written bytes
 */
ssize_t send_message(int fd, char *msg);

/**
 * retrieve_message() - reads a message from the group device.
 * 
 * @fd: the file descriptor
 * @buf: the memory location in which the message will be retrieved
 * @length: the size of the memory location
 * 
 * Reads @legth bytes of a message according to the FIFO policy
 * from the group device related to the file descriptor fd. The
 * read message is stored in @buf.
 * 
 * Returns:
 * -1   - error
 * >= 0 - number of read bytes
 */
ssize_t retrieve_message(int fd, char *buf, size_t length);

/**
 * sleep_on_barrier() - thread sleeps.
 * 
 * @fd: the file descriptor
 * 
 * Puts the calling thread into sleep and raises the barrier. The
 * barrier will stay raised up until a thread wants to awaken
 * those threads sleeping on the group device.
 * 
 * Returns:
 * 0    - ok
 * -1   - ko
 */
int sleep_on_barrier(int fd);

/**
 * awake_barrier() - awakes all sleeping threads of the group device.
 * 
 * @fd: the file descriptor
 * 
 * Awakes all threads sleeping on the group device. In particular, all
 * and only those threads sleeping on that specific group device are
 * awakened. Threads will wake up if the barrier is not raised up.
 * Indeed, the first operation the function performs is to destroy the
 * barrier.
 * 
 * Returns:
 * 0    - ok
 * -1   - ko
 */
int awake_barrier(int fd);

/**
 * set_send_delay() - message writing delay is set.
 * 
 * @fd: the file descriptor
 * @delay: the delay after which messages will be available
 * 
 * Sets the delay to @delay for the group device related to the file
 * descriptor @fd. Non-negative values for the delay are allowed.
 * 
 * Returns:
 * 0    - ok
 * -1   - ko
 */
int set_send_delay(int fd, long delay);

/**
 * revoke_delayed_messages() - publishes all delayed messages.
 * 
 * @fd: the file descriptor
 * 
 * If previously a delay was set over a group device, then incoming 
 * messages are published just as the delay expires. This function 
 * will flush all pending messages, i.e. all messages which are 
 * still waiting to be available are published. 
 * Notice that the delay is unmodified: further incoming messages 
 * will be not immediatly available.
 * 
 * Returns:
 * 0    - ok
 * -1   - ko
 */
int revoke_delayed_messages(int fd);

/**
 * close_group() - closes a group device.
 * 
 * @fd: the file descriptor
 * 
 * The file descriptor @fd is closed.
 * 
 * Returns:
 * void
 */
void close_group(int fd);