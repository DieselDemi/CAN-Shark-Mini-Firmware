#include "can_bus.h"
#include "comms.h"

void can_bus_init(settings_t settings) { 
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(TAG, "Driver installed");
    

    send_data(CAN_DRIVER_INSTALLED);
    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(TAG, "Driver started");
    send_data(CAN_DRIVER_STARTED);
}

int can_bus_update() { 
    twai_message_t message;     
    
    if(!(twai_receive(&message, 1) == ESP_OK)) {
        send_data(CAN_NO_MESSAGE); 
        return 1; 
    }

    send_data(CAN_MESSAGE_START); 
    send_data(message.identifier);

    if(!(message.rtr)) { 
        for(int i = 0; i < message.data_length_code; i++) { 
            send_data(message.data[i]); 
        }
    } else { 
        send_data(CAN_REMOTE_FRAME); 
    }

    send_data(CAN_END_OF_MESSAGE);
    

    return 1; 
}

void can_bus_cleanup() { 
    //Stop and uninstall TWAI driver
    ESP_ERROR_CHECK(twai_stop());
    ESP_LOGI(TAG, "Driver stopped");
    send_data(CAN_DRIVER_STOPPED);
    ESP_ERROR_CHECK(twai_driver_uninstall());
    ESP_LOGI(TAG, "Driver uninstalled");
    send_data(CAN_DRIVER_UNINSTALLED);
}