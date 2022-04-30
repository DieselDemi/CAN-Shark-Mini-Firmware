#ifndef _COMMS_H_
#define _COMMS_H_

#include "driver/uart.h"
#include "driver/gpio.h"

static const int RX_BUF_SIZE = 1024;
static const int TX_BUF_SIZE = 2048; //TODO(Demi): Calculate the maximum message size to be sent to the host pc

char* com_buffer; 

void comms_update_tx(); 
void comms_update_rx(uint8_t *data); 

void copy_to_comms_buffer(char* data);

esp_err_t comms_init(); 
int send_data(char* data); 
int send_data_with_length(char* data, size_t len);

void clear_screen(); 

#endif