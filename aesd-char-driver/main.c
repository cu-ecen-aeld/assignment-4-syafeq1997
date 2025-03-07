/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Your Name Here"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    //retrive pointer of aesd_dev struct that contain specific member 'cdev'
    filp->private_data = container_of(inode->i_cdev, struct aesd_dev, cdev);
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    ssize_t retval = 0;
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry *buff_entry;
    size_t off;
    PDEBUG("write %zu bytes with offset %lld", count, *f_pos);
    /**
     * TODO: handle read
     */


    if(mutex_lock_interruptible(&dev->mutex)){
        return -ERESTARTSYS;
    } 

    buff_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->cbuf, *f_pos, &off);
    if (buff_entry)
    {
        ssize_t sz = buff_entry->size - off;
	if (sz > count)
	{
	    sz = count;
	}

	if (copy_to_user(buf, buff_entry->buffptr + off, sz) != 0)
	{
	    retval = -EFAULT;
	    goto exit;
	}
	*f_pos += sz;
	retval = sz;
    }

    exit:
        mutex_unlock(&dev->mutex);
        return retval;

}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    struct aesd_dev *dev = filp->private_data;
    char *new_buf;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */

    if (mutex_lock_interruptible(&dev->mutex))
    {
        return -ERESTARTSYS;
    }

    new_buf = kmalloc(dev->bufferentry.size + count, GFP_KERNEL);

    if(!new_buf)
    {
        retval = -ENOMEM;
        goto exit;
    }

    //copy buffer to new buffer
    if(dev->bufferentry.size)
    {
        memcpy(new_buf, dev->bufferentry.buffptr, dev->bufferentry.size);
    }

    if(copy_from_user(new_buf + dev->bufferentry.size, buf, count))
    {
        retval = -EFAULT;
        goto exit;
    }

    kfree(dev->bufferentry.buffptr);
    dev->bufferentry.buffptr = new_buf;
    dev->bufferentry.size += count;
    retval = count;


    while (dev->bufferentry.size > 0)
    {
        char *p = memchr(dev->bufferentry.buffptr, '\n', dev->bufferentry.size);
        struct aesd_buffer_entry entry;
	    size_t sz;

	    if (!p)
	    {
	        break;
	    }

	    sz = p + 1 - dev->bufferentry.buffptr;
	    entry.buffptr = dev->bufferentry.buffptr;
	    entry.size = sz;
	    //kfree(aesd_circular_buffer_add_entry(&dev->cbuf, &entry));

	    if (dev->bufferentry.size == sz)
	    {
	        dev->bufferentry.buffptr = NULL;
	        dev->bufferentry.size = 0;
	        break;
	    }
	    else
	    {
	        new_buf = kmemdup(dev->bufferentry.buffptr + sz, dev->bufferentry.size - sz, GFP_KERNEL);

	        if (!new_buf)
	        {
	            retval = -ENOMEM;
		        goto exit;
	        }

	    dev->bufferentry.buffptr = new_buf;
	    dev->bufferentry.size -= sz;
	}
    }
    exit:
        mutex_unlock(&dev->mutex);
        return retval;
}

//loff_t aesd_llseek(struct file *filp, loff_t offset, int whence)
//{
//    loff_t retval;
//    loff_t total_size = 0;
//    struct aesd_dev *ad = filp->private_data;
//
//    PDEBUG("Seek %llu bytes using whence=%d", offset, whence);
//
//    if (mutex_lock_interruptible(&ad->mutex))
//    {
//        return -ERESTARTSYS;
//    }
//
//    total_size = (loff_t) aesd_get_total_size(&ad->cbuf);
//    retval = fixed_size_llseek(filp, offset, whence, total_size);
//
//    mutex_unlock(&ad->mutex);
//
//    return retval;
//}
//
//long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
//{
//    loff_t offset = 0;
//    struct aesd_dev *ad = filp->private_data;
//    struct aesd_seekto seekto;
//
//    PDEBUG("Running ioctl command=%u", cmd);
//
//    if (cmd != AESDCHAR_IOCSEEKTO)
//    {
//        return -ENOTTY;
//    }
//
//    if (copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto)) != 0)
//    {
//        return -EFAULT;
//    }
//
//    PDEBUG("Seekto=%d, offset=%d", seekto.write_cmd, seekto.write_cmd_offset);
//
//    if (mutex_lock_interruptible(&ad->mutex))
//    {
//        return -ERESTARTSYS;
//    }
//
//    offset = (off_t) aesd_get_offset(&ad->cbuf, seekto.write_cmd, seekto.write_cmd_offset);
//
//    if (offset < 0)
//    {
//        offset = -EINVAL;
//        goto exit;
//    }
//
//    filp->f_pos = offset;
//
//exit:
//    mutex_unlock(&ad->mutex);
//    return offset;
//}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release
    //.llseek         = aesd_llseek,
    //.unlocked_ioctl = aesd_ioctl
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));
    
    
    /**
     * TODO: initialize the AESD specific portion of the device
     */
    mutex_init(&aesd_device.mutex);
    aesd_circular_buffer_init(&aesd_device.cbuf);
    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
