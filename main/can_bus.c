#include "can_bus.h"
#include "comms.h"

esp_err_t can_bus_init(can_config_t settings) { 
    esp_err_t last_err = ESP_OK; 
    last_err = twai_driver_install(&settings.g_config, &settings.t_config, &settings.f_config);
    ESP_LOGI(TAG, "Driver installed");
    
    last_err = twai_start();
    ESP_LOGI(TAG, "Driver started");
    // send_data(CAN_DRIVER_STARTED);

    return last_err; 
}

int can_bus_update() { 
    twai_message_t message;     
    
    if(!(twai_receive(&message, 1) == ESP_OK)) {
        copy_to_comms_buffer("NO MSG");
        // send_data(CAN_NO_MESSAGE); 
        return 1; 
    }

    // send_data(CAN_MESSAGE_START); 
    send_data(message.identifier);

    if(!(message.rtr)) { 
        for(int i = 0; i < message.data_length_code; i++) { 
            send_data(message.data[i]); 
        }
    } else { 
        // send_data(CAN_REMOTE_FRAME); 
    }

    // send_data(CAN_END_OF_MESSAGE);
    

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