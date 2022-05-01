#include "can_bus.h"
#include "comms.h"

#include <string.h>

long long microsecond_time; 

comms_message_t generate_message(long long time, can_message_type message_type, uint32_t id, void* data, size_t data_len); 

esp_err_t can_bus_init(can_config_t settings) { 
    esp_err_t last_err = ESP_OK; 
    last_err = twai_driver_install(&settings.g_config, &settings.t_config, &settings.f_config);
    ESP_LOGI(TAG, "Driver installed");
    
    last_err = twai_start();
    ESP_LOGI(TAG, "Driver started");
    // send_data(CAN_DRIVER_STARTED);

    microsecond_time = 0; 

    return last_err; 
}

long long last_time = 0; 

int can_bus_update(bool update) {
    if(!update)
        return 1; 

    twai_message_t message;     
    comms_message_t com_message;

    if(!(twai_receive(&message, CAN_TICKS_TO_WAIT) == ESP_OK)) {
        return 1; 
    }

    if(!message.rtr) {
        microsecond_time = esp_timer_get_time(); 

        com_message = generate_message(microsecond_time - last_time, STANDARD_FRAME, message.identifier, message.data, message.data_length_code);
        add_message(com_message);

#ifdef CAN_DEBUG
        for(int i = 0; i < message.data_length_code; i++) { 
            printf("%i ", message.data[i]);  
        }
        printf("\n"); 
#endif
        last_time = microsecond_time; 
    } else { 
        //TODO(Demi): Send remote frame status
        microsecond_time = esp_timer_get_time(); 

        com_message = generate_message(microsecond_time - last_time, REMOTE_FRAME, message.identifier, NULL, 0);
        add_message(com_message);

        last_time = microsecond_time; 
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

/**
 * @brief Generate a message from can data
 * 
 * @param time 
 * @param id 
 * @param data 
 * @param data_len 
 * @return comms_message_t 
 */
comms_message_t generate_message(long long time, can_message_type message_type, uint32_t id, void* data, size_t data_len) { 
    comms_message_t message; 

    uint16_t type = (uint16_t)message_type;

    uint8_t type_arr[2]; 
    type_arr[0] = (type >> 8) & 0xff; 
    type_arr[1] = (type) & 0xff; 

    uint8_t id_arr[4]; 
    id_arr[0] = (id >> 24) & 0xff; 
    id_arr[1] = (id >> 16) & 0xff; 
    id_arr[2] = (id >> 8) & 0xff; 
    id_arr[3] = (id) & 0xff; 

    uint8_t time_arr[8]; 
    time_arr[0] = (time >> 56) & 0xff; 
    time_arr[1] = (time >> 48) & 0xff; 
    time_arr[2] = (time >> 40) & 0xff; 
    time_arr[3] = (time >> 32) & 0xff; 
    time_arr[4] = (time >> 24) & 0xff; 
    time_arr[5] = (time >> 16) & 0xff; 
    time_arr[6] = (time >> 8) & 0xff; 
    time_arr[7] = (time) & 0xff;

    uint8_t data_arr[data_len + 14]; 

    memcpy(data_arr, time_arr, 8);
    memcpy(data_arr + 8, type_arr, 2);  
    memcpy(data_arr + 10, id_arr, 4); 
    memcpy(data_arr + 14, data, data_len); 

    create_message(&message, data_arr, data_len + 14); 

    return message; 
}
