#ifndef _CAN_BUS_H
#define _CAN_BUS_H

#include "defines.h"

#include <stdio.h>
#include <stdlib.h> 
#include <stdbool.h>
#include <freertos/FreeRTOS.h> 
#include <freertos/task.h> 
#include <freertos/queue.h>
#include <freertos/semphr.h> 
#include <esp_err.h>
#include <esp_log.h>
#include <driver/gpio.h> 
#include <driver/twai.h>

#define TAG "CAN BUS"

typedef enum can_message_type { 
    STANDARD_FRAME = 0, 
    REMOTE_FRAME = 1
} can_message_type; 

typedef struct can_config_t { 
    twai_general_config_t g_config; 
    twai_timing_config_t t_config; 
    twai_filter_config_t f_config; 
} can_config_t; 

static const twai_general_config_t default_g_config = { 
    .mode = TWAI_MODE_LISTEN_ONLY, 
    .rx_io = CAN_BUS_RX_GPIO_NUM, 
    .tx_io = CAN_BUS_TX_GPIO_NUM, 
    .clkout_io = TWAI_IO_UNUSED, 
    .bus_off_io = TWAI_IO_UNUSED,
    .rx_queue_len = 10, 
    .tx_queue_len = 10, 
    .alerts_enabled = TWAI_ALERT_ALL, 
    .clkout_divider = 0
};

static const twai_timing_config_t default_t_config = TWAI_TIMING_CONFIG_500KBITS(); 
static const twai_filter_config_t default_f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL(); 

esp_err_t can_bus_init(can_config_t setting); 
esp_err_t can_bus_update(); 
esp_err_t can_bus_cleanup(); 

#endif