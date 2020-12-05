//
// Created by student on 05/12/2020.
//

#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/list.h>      /* for maintaining all devices */

MODULE_LICENSE("GPL");

//Our custom definitions of IOCTL operations, and more
#include "message_slot.h"

//--------------------------Global variables and initializations ----------------//
struct chardev_info
    //As we seen on recitation 5 + 6 - taking it as is
{
    spinlock_t lock;
};


struct msg
   //For each message, we will maintain a struct with the complete message and message size
{
    char buffer[BUF_LEN];
    int size;
};

typedef struct slot_config
{
    //We will have at most 256 minor numbers (major = 240 hard coded), when each can be up to 2^20 unsigned long.
    int minor;
    unsigned long channel_id;
} slot_config_t;

// used to prevent concurent access into the same device
static int dev_open_flag = 0;

static struct chardev_info device_info;


//==================== DEVICE SETUP =============================
// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
        {
                .owner	  = THIS_MODULE,
                .read           = device_read,
                .write          = device_write,
                .open           = device_open,
                .unlocked_ioctl = device_ioctl,
                .release        = device_release,
        };

//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
    unsigned long flags; // for spinlock
    printk("Invoking device_open(%p)\n", file);

    // We don't want to talk to two processes at the same time
    spin_lock_irqsave(&device_info.lock, flags);
    if( 1 == dev_open_flag )
    {
        spin_unlock_irqrestore(&device_info.lock, flags);
        return -EBUSY;
    }

    ++dev_open_flag;
    spin_unlock_irqrestore(&device_info.lock, flags);
    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
    unsigned long flags; // for spinlock
    printk("Invoking device_release(%p,%p)\n", inode, file);

    // ready for our next caller
    spin_lock_irqsave(&device_info.lock, flags);
    --dev_open_flag;
    spin_unlock_irqrestore(&device_info.lock, flags);
    return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file,
                            char __user* buffer,
size_t       length,
        loff_t*      offset )
{
// read doesnt really do anything (for now)
printk( "Invocing device_read(%p,%ld) - "
"operation not supported yet\n"
"(last written - %s)\n",
file, length, the_message );
//invalid argument error
return -EINVAL;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
size_t             length,
        loff_t*            offset)
{
int i;
printk("Invoking device_write(%p,%ld)\n", file, length);
for( i = 0; i < length && i < BUF_LEN; ++i )
{
get_user(the_message[i], &buffer[i]);
if( 1 == encryption_flag )
the_message[i] += 1;
}

// return the number of input characters used
return i;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
    // Switch according to the ioctl called
    if( MSG_SLOT_CHANNEL == ioctl_command_id )
    {
        if (ioctl_param > 0) {
            //Command is MSG_SLOT_CHANNEL and PARAM > 0
            printk("Invoking ioctl: setting message channel to %ld\n", ioctl_param);
            // DO SOMETHING INTERNAL !! //
        } else {
            errno(EINVAL);
            return FAILURE;
        }
    }
    else {
        //Unsupported command
        errno(EINVAL);
        return FAILURE;
    }
    return FAILURE;
}

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
    int register_result = -1;
    // init dev struct
    memset( &device_info, 0, sizeof(struct chardev_info) );
    spin_lock_init( &device_info.lock );

    // Register driver capabilities. Obtain major num
    register_result = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

    // Negative values signify an error
    if( register_result < 0 )
    {
        printk( KERN_ERROR "%s registraion failed for  %d\n",
                DEVICE_FILE_NAME, MAJOR_NUM );
        return FAILURE;
    }
    else {
        printk( "Registeration is successful. ");
        return SUCCESS;
    }
//    printk( "If you want to talk to the device driver,\n" );
//    printk( "you have to create a device file:\n" );
//    printk( "mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM );
//    printk( "You can echo/cat to/from the device file.\n" );
//    printk( "Dont forget to rm the device file and "
//            "rmmod when you're done\n" );

}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
    // Unregister the device
    // Should always succeed
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);


//======================= INTERNAL FUNCTIONS ====================


//========================= END OF FILE =========================
