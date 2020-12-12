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

struct list_head { /* kernel linked list data structure */
    struct list_head *next, *prev;
};

struct message_channel
   //For each message, we will maintain a struct with the complete message
   //Each message is up to BUF_LEN = 128 bytes, and we need to read/write atomically
   //msg_size holds the actual message size (<=128)
{
    char buffer[BUF_LEN];
    int msg_size;
    unsigned long channel_id;
    struct list_head list; /* add list_head instead of prev and next */
};

typedef struct slot_config
{
    //We will have at most 256 minor numbers (major = 240 hard coded), when each can be up to 2^20 unsigned long.
    int minor;
    unsigned long channel_id;
    struct message_channel *message_channel_head;
} slot_config_t;

// used to prevent concurrent access into the same device
static int dev_open_flag = 0;



//Create a fixed size array of structure message
//Slots number is bound by MAX_MINOR_NUMBER = 256
struct slot_config * slots__array[MAX_MINOR_NUMBER];



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
    struct message_channel *message_channel = get_message_from_file(file);
    int i;

    if (message == NULL || buffer == NULL) {
        perror(EINVAL);
        return(FAILURE);
    }
    if (length > BUF_LEN || length == 0) {
        perror(EMSGSIZE);
        return(FAILURE);
    }

    for (i = 0; i < length && i < BUF_LEN; ++i)
    {
        get_user(message->buffer[i], &buffer[i]);
    }

    if (i == length) {
        //Save how much bytes did we actually load into buffer
    message->size = i;
    }
    else {
     //Didn't succeed writing the complete message
    // Make writing an atomic operation
    message->size = 0;
    }
    //Return #bytes loaded to buffer
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



//======================= INTERNAL FUNCTIONS ====================
// Get a message struct pointer given a file, create it if not matching one is found.
struct message *get_message_from_file(struct file *file)
{

    //struct radix_tree_root *rt_root = minor_containers[minor];

    struct message *message;

    //if (channel_id == 0)
        //return NULL;

    // Try and get message from the tree if already exists
    message = (struct message *)radix_tree_lookup(rt_root, channel_id);

    // Create it if it does not
    if (message == NULL)
    {
        message = kcalloc(1, sizeof(struct message), GFP_KERNEL);
        if (!message)
            return NULL;
        message->size = 0;
        radix_tree_insert(rt_root, channel_id, message);
    }
    return message;
}

//Logic: When gets a slot ID, go to the relevant array index and looks for the message ID in it's linked list
//If there is no such list, create it
struct msg *load_message(struct file *file) {
    //Read minor and channel_id via built in functions
    int minor = ((slot_config_t *)file->private_data)->minor;
    unsigned long channel_id = ((slot_config_t *)file->private_data)->channel_id;
    struct list_head *message_list_ptr = list_head;
    struct message_channel *message_channel ;

    if (channel_id == 0) {
        return NULL;
    }
    //Get the channel and message  node from the slots_array
    message_channel = slots_array[channel_id] ;
    if (message_channel->list.next == NULL) {
        //No data structure available for this device minor number, init a list linked to his minor id
        //INIT_LIST_HEAD(&message->list);
        message_channel = kcalloc(1, sizeof(struct message_channel), GFP_KERNEL);
        {if (!message) return NULL ;}
        message_channel->list = LIST_HEAD_INIT(message_channel->list);
    }
    else {
        //Data structure linked to it's minor id exists, add to its linked list
        list_add(&message_channel->list_head, &(message_channel->list));

    }

    // Create it if it does not
    if (message == NULL)
    {
        if (message_list_ptr->next == NULL) {
            //List isn't initialized yet, create one
            message = kcalloc(1, sizeof(struct message), GFP_KERNEL);
            {if (!message) return NULL} ;
            INIT_LIST_HEAD(&message->list);
        }
        else{
            //List exists, we need to add
            list_add(&message->list_head, &list);
        }
    }

}


//==================== DEVICE SETUP =============================
// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
        {
                .owner = THIS_MODULE,
                .read = device_read,
                .write = device_write,
                .open = device_open,
                .release = device_release,
                .unlocked_ioctl = device_ioctl,

        };
//========================= END OF FILE =========================
module_init(simple_init);
module_exit(simple_cleanup);