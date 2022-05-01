#include "can_bus.h"
#include "comms.h"

#include <string.h>

/// Private variables
long long microsecond_time; 
long long last_time; 

esp_err_t last_err; 

/// Private function pre declarations
esp_err_t generate_message(comms_message_t *message, long long time, can_message_type message_type, uint32_t id, void* data, size_t data_len); 

/**
 * @brief Initialize the CAN Bus driver
 * 
 * @param settings 
 * @return esp_err_t 
 */
esp_err_t can_bus_init(can_config_t settings) {  
    twai_status_info_t status_info; 
    
    
    //Set the log level
#ifdef CAN_DEBUG
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
#else
    esp_log_level_set(TAG, ESP_LOG_NONE);
#endif
    
    //Get the current twai state return if something went wrong or already running
    last_err = twai_get_status_info(&status_info); 
    if((last_err != ESP_OK && last_err != ESP_ERR_INVALID_STATE) || status_info.state == TWAI_STATE_RUNNING) 
        return last_err; 
        
    //Install the driver based on the current provided configuration
    last_err = twai_driver_install(&settings.g_config, &settings.t_config, &settings.f_config);

    ESP_LOGI(TAG, "Driver installed");

    //Start the CAN bus driver
    last_err = twai_start();

    ESP_LOGI(TAG, "Driver started");

    //Initialize our time variables
    microsecond_time = 0; 
    last_time = 0; 

    return last_err; 
}

/**
 * @brief Update the can bus 
 * 
 * @return esp_err_t 
 */
esp_err_t can_bus_update() {
    twai_message_t message;     
    comms_message_t com_message;

    last_err = ESP_OK; 

    if(!(last_err = twai_receive(&message, CAN_TICKS_TO_WAIT) == ESP_OK)) {
        return ESP_OK; 
    }

    //If the message is not a remote CAN frame, send the regular data
    //else send a message containing just the time, ID, and type.  
    if(!message.rtr) {
        microsecond_time = esp_timer_get_time(); 

        generate_message(&com_message, microsecond_time - last_time, STANDARD_FRAME, message.identifier, message.data, message.data_length_code);
        add_message(com_message);

// #ifdef CAN_DEBUG
//         for(int i = 0; i < message.data_length_code; i++) { 
//             printf("%i ", message.data[i]);  
//         }
//         printf("\n"); 
// #endif
        last_time = microsecond_time; 
    } else { 
        microsecond_time = esp_timer_get_time(); 

        generate_message(&com_message, microsecond_time - last_time, REMOTE_FRAME, message.identifier, NULL, 0);
        add_message(com_message);

        last_time = microsecond_time; 
    }    

    return ESP_OK; 
}

/**
 * @brief Cleanup the CAN bus driver
 * 
 * @return esp_err_t Error response
 */
esp_err_t can_bus_cleanup() { 
    twai_status_info_t status_info; 
    
    //Get the current twai state, and if its already stopped ignore don't try to
    //stop it again.  
    last_err = twai_get_status_info(&status_info); 
    
    if(last_err == ESP_ERR_INVALID_STATE)
        return ESP_OK; 
    
    if(last_err != ESP_OK || status_info.state == TWAI_STATE_STOPPED)
        return last_err; 

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
 * @brief Generate a comms message from CAN_BUS data
 * 
 * @param message 
 * @param time 
 * @param message_type 
 * @param id 
 * @param data 
 * @param data_len 
 * @return esp_err_t 
 */
esp_err_t generate_message(comms_message_t *message, long long time, can_message_type message_type, uint32_t id, void* data, size_t data_len) { 
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

    create_message(message, data_arr, data_len + 14); 

    return ESP_OK; 
}
