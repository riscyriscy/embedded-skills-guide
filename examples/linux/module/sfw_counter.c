// SPDX-License-Identifier: GPL-2.0
/* A character device, /dev/sfw_counter, that returns the number of times it was read.
   It shows the parts of a small driver: a module parameter, a file_operations table,
   registration with cleanup on failure, a lock, and the copy to user space. */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>

static unsigned int start;
module_param(start, uint, 0444); /* read only in /sys/module/sfw_counter/parameters */
MODULE_PARM_DESC(start, "first value that a read returns");

static unsigned int bad;
module_param(bad, uint, 0444); /* 1: read dereferences a null pointer, for an oops */
MODULE_PARM_DESC(bad, "set to 1 to make read fail with a kernel oops");

static int *no_buffer; /* stays NULL */
static DEFINE_MUTEX(lock);
static unsigned int counter;

static ssize_t sfw_read(struct file *file, char __user *buf, size_t len, loff_t *pos)
{
    char text[16];
    int n;

    if (*pos > 0) {
        return 0; /* end of file after the first line */
    }
    mutex_lock(&lock);
    if (bad) {
        counter += *no_buffer; /* null pointer dereference */
    }
    n = scnprintf(text, sizeof(text), "%u\n", counter++);
    mutex_unlock(&lock);
    if (len < n) {
        return -EINVAL;
    }
    if (copy_to_user(buf, text, n)) {
        return -EFAULT;
    }
    *pos += n;
    return n;
}

static const struct file_operations sfw_fops = {
    .owner = THIS_MODULE,
    .read = sfw_read,
};

static struct miscdevice sfw_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "sfw_counter",
    .fops = &sfw_fops,
};

static int __init sfw_init(void)
{
    int err;

    counter = start;
    err = misc_register(&sfw_dev);
    if (err) {
        pr_err("sfw_counter: misc_register failed: %d\n", err);
        return err;
    }
    pr_info("sfw_counter: loaded, first value %u\n", start);
    return 0;
}

static void __exit sfw_exit(void)
{
    misc_deregister(&sfw_dev);
    pr_info("sfw_counter: unloaded after %u reads\n", counter - start);
}

module_init(sfw_init);
module_exit(sfw_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Example character device that counts reads");
