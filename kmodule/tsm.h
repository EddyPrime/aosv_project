#pragma once

#define TSM_MAJOR 0

#define TSM_NAME "tsm"
#define TSM_CLASS_NAME "tsm_class"

#define DEFAULT_MAX_MESSAGE_SIZE 32
#define DEFAULT_MAX_STORAGE_SIZE 64

struct file_operations tsm_dev_fops;

static int tsm_dev_open(struct inode *inode, struct file *filp);
static int tsm_dev_release(struct inode *inode, struct file *filp);
static long tsm_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);