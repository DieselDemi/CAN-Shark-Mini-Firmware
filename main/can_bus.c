#include "can_bus.h"
#include "comms.h"
#include "esp_timer.h"
#include <string.h>
#include <lwip/sockets.h>

/// Private variables
int64_t microsecond_time; 
int64_t last_microsecond_time; 

esp_err_t last_err; 

/// Private function pre declarations
esp_err_t generate_message(comms_message_t *message, uint32_t time, can_message_type message_type, uint32_t id, void* data, size_t data_len); 

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
    last_microsecond_time = 0; 

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

    microsecond_time = esp_timer_get_time(); 
    last_err = generate_message(&com_message, microsecond_time - last_microsecond_time, message.rtr ? REMOTE_FRAME : STANDARD_FRAME, message.identifier, message.data, message.data_length_code); 
    add_message(&com_message); 
    last_microsecond_time = microsecond_time; 

    return last_err; 
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
esp_err_t generate_message(comms_message_t *message, uint32_t time, can_message_type message_type, uint32_t id, void* data, size_t data_len) { 
    //NOTE: This could be done with a few memcpy's however I think by unrolling the loop and bitwise shifting its actually slightly faster

    // Translate the message type and convert it to a byte array
    size_t type_arr_len = sizeof(uint16_t);
    uint8_t type_arr[type_arr_len]; 
    uint16_t net_type = htons((uint16_t)message_type); // Convert to network byte order
    memcpy(type_arr, &net_type, type_arr_len); 

    // Convert the ID to a byte array
    size_t id_arr_len = sizeof(uint32_t);
    uint8_t id_arr[id_arr_len]; 
    uint32_t net_id = htonl(id); 
    memcpy(id_arr, &net_id, id_arr_len); 

    // Convert the time to a byte array
    size_t time_arr_len = sizeof(uint32_t); 
    uint8_t time_arr[time_arr_len]; 
    long net_time = htonl(time); 
    memcpy(time_arr, &net_time, time_arr_len); 

    size_t message_data_arr_len = type_arr_len + id_arr_len + time_arr_len + data_len; 
    uint8_t message_data_arr[message_data_arr_len]; 
    memset(message_data_arr, 0, message_data_arr_len);

    memcpy(message_data_arr, time_arr, time_arr_len);
    memcpy(message_data_arr + time_arr_len, type_arr, type_arr_len);  
    memcpy(message_data_arr + time_arr_len + type_arr_len, id_arr, id_arr_len); 
    memcpy(message_data_arr + time_arr_len + type_arr_len + id_arr_len, data, data_len); 
    
    // printf("create_message(message, message_data_arr, %d);\n", message_data_arr_len);
    return create_message(message, message_data_arr, message_data_arr_len); 
}
