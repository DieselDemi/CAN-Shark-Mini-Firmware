#ifndef _COMMS_H_
#define _COMMS_H_

//Driver Status
#define CAN_DRIVER_STARTED 0x01 
#define CAN_DRIVER_STOPPED 0x02
#define CAN_DRIVER_INSTALLED 0x03
#define CAN_DRIVER_UNINSTALLED 0x04

#define CAN_NO_MESSAGE 0x10
#define CAN_REMOTE_FRAME 0x11

#define CAN_MESSAGE_START 0x15

#define CAN_END_OF_MESSAGE 0x1F

typedef struct settings_t { 
    int speed;
    int filter;  
} settings_t; 

void send_data(int data);

settings_t get_settings(); 

#endif