#include "message_slot.h"


#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ********************CHANGE THE LOGIC****************************//

int main(int argc, char *argv[])
{
//    int file_desc;
//    int ret_val;
//    char buffer[BUF_LEN];
//    unsigned long channel_id;
//
//    if (argc < 3) {
//        fprintf(stderr, PARAM_ERROR);
//        exit(1);
//    }
//
//    if( file_desc < 0 ) {
//        perror(OPEN_ERROR);
//        exit(1);
//    }
//
//    if (ret_val != SUCCESS) {
//        perror(IOCTL_ERROR);
//        exit(1);
//    }
//
//    if (ret_val < 0) {
//        perror(READ_ERROR);
//        exit(1);
//    } else {
//        if (write(STDOUT_FILENO, buffer, ret_val) == -1) {
//            perror(STDOUT_ERROR);
//        }
//    }
//    close(file_desc);
    return 0;
}
