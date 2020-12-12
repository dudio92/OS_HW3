//
// Created by student on 05/12/2020.
//

#ifndef message_slot
#define message_slot
#include <linux/ioctl.h>

//Predefine major num for ioctl //
#define MAJOR_NUM 240

// Set the message of the device driver
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

#define DEVICE_RANGE_NAME "message_slot"
#define BUF_LEN 128
#define MAX_MINOR_NUMBER 256
#define DEVICE_FILE_NAME "slot0"
#define SUCCESS 0
#define FAILURE -1

// Error printing and more
#define IOCTL_ERROR "IOCTL failed\n"
#define PARAM_ERROR "At least on argument is missing, Please specify all parameters\n"
#define OPEN_ERROR "descriptor open error"
#define WRITE_ERROR "Device write error"
#define READ_ERROR "Device read error"
#define STDOUT_ERROR "Writing to STDOUT failed\n"

#endif
