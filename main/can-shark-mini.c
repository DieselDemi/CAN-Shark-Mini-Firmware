#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "defines.h"
#include "comms.h"
#include "can_bus.h"
#include "ota.h"

TaskHandle_t sniff_handle;

can_config_t can_bus_config = { 
    .g_config = default_g_config, 
    .t_config = default_t_config, 
    .f_config = default_f_config
};

comms_status_t prog_status = {
    .sniff = false,
    .current_config = {
        .g_config = default_g_config, 
        .t_config = default_t_config, 
        .f_config = default_f_config
    } 
}; 

void init(void) {
    ESP_ERROR_CHECK(comms_init()); 
    ESP_ERROR_CHECK(ota_do_after_update());

    //clear_screen();
}

static void can_bus_task(void *arg) { 
    // ESP_ERROR_CHECK(can_bus_init(can_bus_config)); 
    while(1) {

        if(!prog_status.sniff)
        {
            ESP_ERROR_CHECK(can_bus_cleanup()); 
        } else { 
            can_bus_config = prog_status.current_config; 
            
            ESP_ERROR_CHECK(can_bus_init(can_bus_config)); 
            ESP_ERROR_CHECK(can_bus_update());
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    ESP_ERROR_CHECK(can_bus_cleanup()); 
}

static void comms_tx_task(void *arg)
{
    while (1) {
        comms_update_tx(); 
        
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

static void comms_rx_task(void *arg)
{
    char* data = (char*) malloc(RX_BUF_SIZE + 1); //Allocate a recieve buffer for the max buffer size plus null
    memset(data, 0, RX_BUF_SIZE + 1); // Clear the data 

    while (1) {
        comms_update_rx(&prog_status, data);
        size_t filledSize = strlen(data); 
        memset(data, 0, filledSize); 
    }

    free(data);
}

void app_main(void)
{
    nvs_flash_init(); 

    init(); 

    xTaskCreatePinnedToCore(can_bus_task, "canbus", 1024*2, NULL, configMAX_PRIORITIES, &sniff_handle, 1); 
    xTaskCreatePinnedToCore(comms_tx_task, "uart_tx_task", 2048*2, NULL, configMAX_PRIORITIES-1, NULL, 0);
    xTaskCreatePinnedToCore(comms_rx_task, "uart_rx_task", 2048*2, NULL, configMAX_PRIORITIES-2, NULL, 0);

    vTaskDelay(10 / portTICK_PERIOD_MS);

    printf("\n");
    printf("READY\n");
}