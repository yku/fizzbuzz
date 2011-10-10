#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <asm/current.h>
#include <asm/uaccess.h>

MODULE_AUTHOR("YKU");
MODULE_DESCRIPTION("FizzBuzz");
MODULE_LICENSE("GPL");

#define DRIVER_NAME "fizzbuzz"

static int fizzbuzz_devs = 2; /* device count */
static int fizzbuzz_major = 0;
module_param(fizzbuzz_major, uint, 0);
static struct cdev fizzbuzz_cdev;
static bool used = false;

struct fizzbuzz_data {
    unsigned int value;
    rwlock_t lock;
};

static int fizzbuzz_open(struct inode *inode, struct file *file)
{
    struct fizzbuzz_data *p;
    printk("%s: major %d minor %d pid %d\n", __func__,
                imajor(inode),
                iminor(inode),
                current->pid
                );
   
    p = kmalloc(sizeof(struct fizzbuzz_data), GFP_KERNEL);
    if(p == NULL) {
        printk("%s: Not memory\n", __func__);
        return -ENOMEM;
    }

    p->value = 100;
    used = false;
    rwlock_init(&p->lock);

    file->private_data = p;

    return 0;
}

static int fizzbuzz_close(struct inode *inode, struct file *file)
{
    printk("%s: major %d minor %d pid %d\n", __func__,
                imajor(inode),
                iminor(inode),
                current->pid
                );
    
    if(file->private_data) {
        kfree(file->private_data);
        file->private_data = NULL;
    }

    return 0;
}

static ssize_t fizzbuzz_write(struct file* filp, const char *buf, size_t count, loff_t *pos)
{
   printk(KERN_ALERT "count %d\n", count);
    return count;
}

static ssize_t fizzbuzz_read(struct file *filp, char *buf, size_t count, loff_t *pos)
{
    struct fizzbuzz_data *p = filp->private_data;
    int i, ret;
    unsigned int value;
   
    if(used) return 0;

    read_lock(&p->lock);
    value = p->value;
    used = true;
    read_unlock(&p->lock);

    for(i = 1; i <= value; i++) {
        int send;
        char val[11];
        
        if(i % 3 == 0 && i % 5 == 0) {
            send = sprintf(val, "%s ", "fizzbuzz");
        }else if(i % 3 == 0) {
            send = sprintf(val, "%s ", "fizz");
        }else if(i % 5 == 0) {
            send = sprintf(val, "%s ", "buzz");
        }else{
            send = sprintf(val, "%d ", i);
        }

        if(copy_to_user(&buf[count], val, send)) {
            printk(KERN_ALERT "copy_to_user failed");
            ret = -EFAULT;
            goto out;
        }
        count += send;
    }
    ret = count;
out:
    return (ret);
}

struct file_operations fizzbuzz_fops = {
    .open = fizzbuzz_open,
    .release = fizzbuzz_close,
    .read = fizzbuzz_read,
    .write = fizzbuzz_write,
};

static int fizzbuzz_init(void)
{
   dev_t dev = MKDEV(fizzbuzz_major, 0);
   int ret = 0;
   int major;
   int cdev_err = 0;

   ret = alloc_chrdev_region(&dev, 0, fizzbuzz_devs, DRIVER_NAME);
   if(ret)
       goto error;
   fizzbuzz_major = major = MAJOR(dev);

   cdev_init(&fizzbuzz_cdev, &fizzbuzz_fops);
   fizzbuzz_cdev.owner = THIS_MODULE;

   cdev_err = cdev_add(&fizzbuzz_cdev, MKDEV(fizzbuzz_major, 0), fizzbuzz_devs);
   if(cdev_err)
       goto error;

   printk(KERN_ALERT "%s driver(major %d) installed\n", DRIVER_NAME, major);

   return 0;

error:
   if(cdev_err == 0)
       cdev_del(&fizzbuzz_cdev);

   if(ret == 0)
       unregister_chrdev_region(dev, fizzbuzz_devs);

   return -1;
}

static void fizzbuzz_exit(void)
{
    dev_t dev = MKDEV(fizzbuzz_major, 0);

    cdev_del(&fizzbuzz_cdev);
    unregister_chrdev_region(dev, fizzbuzz_devs);

    printk(KERN_ALERT "%s driver removed\n", DRIVER_NAME);
}

module_init(fizzbuzz_init);
module_exit(fizzbuzz_exit);
