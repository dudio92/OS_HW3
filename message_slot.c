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
#include <linux/slab.h>

MODULE_LICENSE("GPL");

//Our custom definitions of IOCTL operations, and more
#include "message_slot.h"


//--------------------------Global variables and initializations ----------------//


struct message_channel
   //For each message, we will maintain a struct with the complete message
   //Each message is up to BUF_LEN = 128 bytes, and we need to read/write atomically
   //msg_size holds the actual message size (<=128)
{
    char buffer[BUF_LEN];
    int msg_size;
    unsigned long channel_id;
    //struct list_head channels_list_head; /* add list_head instead of prev and next */
    struct list_head *message_channel_head;
};

typedef struct slot_config
{
    //We will have at most 256 minor numbers (major = 240 hard coded), when each can be up to 2^20 unsigned long.
    int minor;
    unsigned long channel_id;
    struct list_head *message_channel_head;
} slot_config_t;

// used to prevent concurrent access into the same device




//Create a fixed size array of structure message
//Slots number is bound by MAX_MINOR_NUMBER = 256
struct slot_config * slots_array[MAX_MINOR_NUMBER];

struct message_channel* get_message_from_list(struct slot_config *slot_config,unsigned long msg_channel_id) {
    struct message_channel *list_message;
    struct list_head *ptr;

    list_for_each(ptr, slot_config->message_channel_head) {
        /* my points to the structure in which the list is embedded */
        list_message = list_entry(ptr, struct message_channel, message_channel_head);
        printk("Slot number %d {Channel ID:%lu, Message:%s}\n", slot_config->minor, list_message->channel_id,
               list_message->buffer);
        if (list_message->channel_id == msg_channel_id) {
            return list_message;
        }
    }
        //Didn't find this channel on this minor, list_message is NULL
        return list_message;

}


struct message_channel *load_message(struct file *file) {
    //We get the minor and channel via the file private data pointer we set on device_open function
    //slot_config already init in device_open
    int minor = ((slot_config_t *)file->private_data)->minor;
    unsigned long channel_id = ((slot_config_t *)file->private_data)->channel_id;

    struct message_channel *get_message_channel ;

    if (channel_id == 0) {
        return NULL;
    }
    get_message_channel = get_message_from_list((slot_config_t *)file->private_data,channel_id) ;


    if (get_message_channel == NULL) {
        //No channel id node exists for this minor
        get_message_channel = kcalloc(1, sizeof(struct message_channel), GFP_KERNEL);
        if (!get_message_channel) {
            return NULL;
        }
        get_message_channel->msg_size = 0;
        list_add(get_message_channel->message_channel_head,slots_array[minor]->message_channel_head);
    }

    return get_message_channel ;
}


slot_config_t* create_slot_config( struct inode *inode,
                                  struct file*  file)
{
    //Init a new slot_config struct and return a pointer to it
    //For memory handling, add the slot_config struct to the private data field of a driver
    //https://www.oreilly.com/library/view/linux-device-drivers/0596000081/ch03s04.html

    slot_config_t *slot_config_node = kmalloc(sizeof(slot_config_t), GFP_KERNEL);
    if (!slot_config_node) {
        //perror(KMALLOC_ERROR);
    }
    slot_config_node->minor = iminor(inode);
    slot_config_node->channel_id = 0;
    //device private data will hold a slot_config struct pointer
    file->private_data = (void *)slot_config_node;

    return slot_config_node;
}



//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
    printk("Invoking device_open(%p)\n", file);
    //When device_open is being called, this function creates new slots_config struct, allocates memory and sets its fields
    //Call slots_config constructor create_slot_config
    slot_config_t *slot_config;
    slot_config = create_slot_config(inode,file);
    // now slot_config struct is ready to be added into slots_array
    //slots_array[slot_config.minor] = &slot_config ;
    // Check if slot_config linked list is initialized, and if not - create it
    if (slots_array[slot_config->minor] == NULL) {
        slots_array[slot_config->minor] = slot_config ;
        //Init it's linked list
        INIT_LIST_HEAD(slots_array[slot_config->minor]->message_channel_head);
    }
    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{


    // ready for our next caller
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
    struct message_channel *message_channel = load_message(file);
    int i;

    if (message_channel == NULL || buffer == NULL) {
        //perror(EINVAL);
        return(FAILURE);
    }
    if (length > BUF_LEN || length == 0) {
        //perror(EMSGSIZE);
        return(FAILURE);
    }

    for (i = 0; i < length && i < BUF_LEN; ++i)
    {
        get_user(message_channel->buffer[i], &buffer[i]);
    }

    if (i == length) {
        //Save how much bytes did we actually load into buffer
        message_channel->msg_size = i;
    }
    else {
     //Didn't succeed writing the complete message
    // Make writing an atomic operation
        message_channel->msg_size = 0;
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
            return FAILURE;
        }
    }
    else {
        //Unsupported command
        return FAILURE;
    }
    return FAILURE;
}

struct file_operations Fops =
        {
                .owner = THIS_MODULE,
                .read = device_read,
                .write = device_write,
                .open = device_open,
                .release = device_release,
                .unlocked_ioctl = device_ioctl,

        };

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
    int register_result = -1;
    // init dev struct


    // Register driver capabilities. Obtain major num
    register_result = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

    // Negative values signify an error
    if( register_result < 0 )
    {
        printk("%s registraion failed for  %d\n",
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
//struct message *get_message_from_file(struct file *file)
//{
//
//    //struct radix_tree_root *rt_root = minor_containers[minor];
//
//    struct message *message;
//
//    //if (channel_id == 0)
//        //return NULL;
//
//    // Try and get message from the tree if already exists
//    message = (struct message *)radix_tree_lookup(rt_root, channel_id);
//
//    // Create it if it does not
//    if (message == NULL)
//    {
//        message = kcalloc(1, sizeof(struct message), GFP_KERNEL);
//        if (!message)
//            return NULL;
//        message->size = 0;
//        radix_tree_insert(rt_root, channel_id, message);
//    }
//    return message;
//}

//Logic: When gets a slot ID, go to the relevant array index and looks for the message ID in it's linked list
//If there is no such list, create it




//==================== DEVICE SETUP =============================
// This structure will hold the functions to be called
// when a process does something to the device we created

//========================= END OF FILE =========================
module_init(simple_init);
module_exit(simple_cleanup);