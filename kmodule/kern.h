#define CLASS_NAME "tsm"
#define GROUP_CLASS_NAME "group_dev_class"

#ifndef DEBUG
#define DEBUG 0
#endif

#define dbg(format, arg...)                                           \
    do                                                                \
    {                                                                 \
        if (DEBUG)                                                    \
            pr_info(CLASS_NAME ": %s: " format, __FUNCTION__, ##arg); \
    } while (0)

#define err(format, arg...)                                      \
    do                                                           \
    {                                                            \
        pr_err(CLASS_NAME ": %s: " format, __FUNCTION__, ##arg); \
    } while (0)

#define info(format, arg...)                                      \
    do                                                            \
    {                                                             \
        pr_info(CLASS_NAME ": %s: " format, __FUNCTION__, ##arg); \
    } while (0)

#define warn(format, arg...)                                      \
    do                                                            \
    {                                                             \
        pr_warn(CLASS_NAME ": %s: " format, __FUNCTION__, ##arg); \
    } while (0)

#define kmalloc_err(s) err("kmalloc %s\n", s)
#define kzalloc_err(s) err("kzalloc %s\n", s)
#define ref_err(s) err("cannot reference %s\n", s)

#define dbg_start() dbg("%s\n", START_MSG)
#define dbg_end() dbg("%s\n", DONE_MSG)

#define info_start() info("%s\n", START_MSG)
#define info_end() info("%s\n", DONE_MSG)

#define CS_START_MSG "begin critical section"
#define CS_DONE_MSG "done critical section"

#define dbg_cs_start() dbg("%s\n", CS_START_MSG)
#define dbg_cs_end() dbg("%s\n", CS_DONE_MSG)
