#ifndef _COMMS_H_
#define _COMMS_H_

#include "driver/uart.h"
#include "driver/gpio.h"
#include "defines.h"
#include "can_bus.h"

typedef struct comms_status_t { 
    bool sniff; 
    can_config_t current_config; 
} comms_status_t; 

typedef struct comms_message_t { 
    uint8_t* data;
    size_t data_length; 
} comms_message_t; 

void comms_update_tx(); 
void comms_update_rx(comms_status_t* status, uint8_t *data); 

void add_message(comms_message_t message); 

esp_err_t create_message(comms_message_t *src, void *data, size_t len); 
esp_err_t comms_init(); 

void clear_screen(); 

#endif