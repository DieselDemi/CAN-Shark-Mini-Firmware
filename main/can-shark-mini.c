#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

#include "defines.h"
#include "comms.h"
#include "can_bus.h"

void init(void) {
    can_config_t base_config = { 
        .g_config = default_g_config, 
        .t_config = default_t_config, 
        .f_config = default_f_config
    };

    ESP_ERROR_CHECK(comms_init()); 
    ESP_ERROR_CHECK(can_bus_init(base_config)); 

    //clear_screen();
}

static void can_bus_task(void *arg) { 
    while(can_bus_update()) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    ESP_ERROR_CHECK(can_bus_cleanup()); 
}

static void comms_tx_task(void *arg)
{
    while (1) {
        comms_update_tx(); 
        

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    free(com_buffer); //Free the com buffer on exit
}

static void comms_rx_task(void *arg)
{
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE + 1); //Allocate a recieve buffer for the max buffer size pluss null

    while (1) {
        comms_update_rx(data);
    }

    free(data);
}

void app_main(void)
{
    init();
    xTaskCreate(can_bus_task, "canbus_sniff_task", 1024*2, NULL, configMAX_PRIORITIES, NULL); 
    xTaskCreate(comms_rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(comms_tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-2, NULL);
}