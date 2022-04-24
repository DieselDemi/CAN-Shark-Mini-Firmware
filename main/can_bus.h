#ifndef _CAN_BUS_H
#define _CAN_BUS_H

#include <stdio.h>
#include <stdlib.h> 
#include <freertos/FreeRTOS.h> 
#include <freertos/task.h> 
#include <freertos/queue.h>
#include <freertos/semphr.h> 
#include <esp_err.h>
#include <esp_log.h>
#include <driver/gpio.h> 
#include <driver/twai.h>
#include "comms.h"

#define TX_GPIO_NUM                     GPIO_NUM_22
#define RX_GPIO_NUM                     GPIO_NUM_21
#define TAG                             "CAN LOGGER"


// static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL(); 
// static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); 
static const twai_general_config_t g_config = { 
    .mode = TWAI_MODE_LISTEN_ONLY, 
    .rx_io = RX_GPIO_NUM, 
    .tx_io = TX_GPIO_NUM, 
    .clkout_io = TWAI_IO_UNUSED, 
    .bus_off_io = TWAI_IO_UNUSED,
    .rx_queue_len = 5, 
    .tx_queue_len = 0, 
    .alerts_enabled = TWAI_ALERT_NONE, 
    .clkout_divider = 0
};

// static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, TWAI_MODE_NORMAL); 
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); 
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL(); 

void can_bus_init(settings_t settings); 
int can_bus_update(); 
void can_bus_cleanup(); 

#endif