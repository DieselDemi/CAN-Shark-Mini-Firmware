#include "can_bus.h"
#include "comms.h"

#include <string.h>

esp_err_t can_bus_init(can_config_t settings) { 
    esp_err_t last_err = ESP_OK; 
    last_err = twai_driver_install(&settings.g_config, &settings.t_config, &settings.f_config);
    ESP_LOGI(TAG, "Driver installed");
    
    last_err = twai_start();
    ESP_LOGI(TAG, "Driver started");
    // send_data(CAN_DRIVER_STARTED);

    return last_err; 
}

int can_bus_update(bool update) {
    if(!update)
        return 1; 

    twai_message_t message;     
    comms_message_t com_message;

    if(!(twai_receive(&message, CAN_TICKS_TO_WAIT) == ESP_OK)) {
        return 1; 
    }

    if(!message.rtr) {
        uint8_t identifier_arr[4]; 
        identifier_arr[0] = (message.identifier >> 24) & 0xff; 
        identifier_arr[1] = (message.identifier >> 16) & 0xff; 
        identifier_arr[2] = (message.identifier >> 8) & 0xff; 
        identifier_arr[3] = (message.identifier) & 0xff; 

        uint8_t complete_arr[message.data_length_code + 4]; 

        memccpy(complete_arr, identifier_arr, 0, 4); 
        memccpy(complete_arr, message.data, 4, message.data_length_code); 

        create_message(&com_message, complete_arr, message.data_length_code + 4); 
        add_message(com_message);

#ifdef CAN_DEBUG
        for(int i = 0; i < message.data_length_code; i++) { 
            printf("%i ", message.data[i]);  
        }
        printf("\n"); 
#endif

    } else { 
        //TODO(Demi): Send remote frame status
    }    

    return 1; 
}

esp_err_t can_bus_cleanup() { 
    esp_err_t last_err; 

    //Stop and uninstall TWAI driver
    last_err = twai_stop();
    
    ESP_LOGI(TAG, "Driver stopped");
    // send_data(CAN_DRIVER_STOPPED);
    last_err = twai_driver_uninstall();

    ESP_LOGI(TAG, "Driver uninstalled");
    // send_data(CAN_DRIVER_UNINSTALLED);
    return last_err; 
}