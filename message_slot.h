//
// Created by student on 05/12/2020.
//

#ifndef message_slot
#define message_slot
#include <linux/ioctl.h>

//Predefine major num for ioctl //
#define MAJOR_NUM 240

// Set the message of the device driver
#define IOCTL_SET_ENC _IOW(MAJOR_NUM, 0, unsigned long)

#define DEVICE_RANGE_NAME "message_slot"
#define BUF_LEN 128
#define DEVICE_FILE_NAME "slot0"
#define SUCCESS 0
#define FAILURE -1

#endif
