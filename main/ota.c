#include "ota.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"

const esp_partition_t *update_partition = NULL; 
esp_ota_handle_t ota_handle; 

esp_err_t last_err; 

bool initialized = false; 

/**
 * @brief Initialize everything to do the update
 * 
 * @return esp_err_t 
 */
esp_err_t ota_init() {
    update_partition = esp_ota_get_next_update_partition(NULL); 
    assert(update_partition != NULL); 

    //Use OTA_SIZE_UNKNOWN to erase the entire partition 
    last_err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle); 

    if(last_err != ESP_OK)
        return last_err; 

    initialized = true;

    return last_err; 
}

/**
 * @brief Do the actual update, this method may be called multiple times as data comes in, if the
 * image does not fit into ram (which it wont... cause ELF). 
 * 
 * It will perform multiple sequential writes every time it is called. 
 * 
 * @param image ptr to data to flash to the new update partition
 * @param size size of the data to be written
 * @return esp_err_t 
 */
esp_err_t ota_do_update(void* image, size_t size) { 
    if(image == NULL || size == 0) 
        return ESP_ERR_IMAGE_INVALID;

    if(!initialized || last_err != ESP_OK)
        return ESP_ERR_INVALID_STATE; 

    return (last_err = esp_ota_write(ota_handle, image, size));
}

/**
 * @brief Cleanup everything do basic checks, then restart the ESP32
 * 
 * TODO(Demi): Clean this up
 */
void ota_cleanup() { 
    if(!initialized)
        last_err = ESP_ERR_INVALID_STATE; 

    if(last_err != ESP_OK) {
        ESP_LOGE("OTA", "SOMETHING WENT WRONG DURING UPDATE");
        return; 
    }

    last_err = esp_ota_end(ota_handle); 

    if(last_err != ESP_OK) {
        ESP_LOGE("OTA", "SOMETHING WENT WRONG DURING UPDATE");
        return; 
    }


    last_err = esp_ota_set_boot_partition(update_partition); 

    if(last_err != ESP_OK) {
        ESP_LOGE("OTA", "SOMETHING WENT WRONG DURING UPDATE");
        return; 
    }
    
    esp_restart(); 
} 

/**
 * @brief This method is called during intial app init no matter what. 
 * It checks the application state, and sets it to valid app. 
 * TODO(): Implement diagnostic tests to validate the freshly flashed application
 * 
 * @return esp_err_t 
 */
esp_err_t ota_do_after_update() { 
    esp_ota_img_states_t img_state; 
    const esp_partition_t *current_partition; 

    current_partition = esp_ota_get_running_partition(); 

    /**
     * TODO(Demi): Maybe use the partition label name to see if 
     * we are in a valid OTA partition
     */

    if(current_partition == NULL) 
        return (last_err = ESP_ERR_INVALID_RESPONSE);

    last_err = esp_ota_get_state_partition(current_partition, &img_state); 

    //If the partition state returns not supported that means we are running
    // a factory app partition, so no need to do anything else. 
    if(last_err == ESP_ERR_NOT_SUPPORTED)
        return ESP_OK; 

    if(last_err != ESP_OK) 
        return last_err; 

    if(img_state == ESP_OTA_IMG_PENDING_VERIFY)
        esp_ota_mark_app_valid_cancel_rollback(); 

    return last_err; 
}


/**
 * @brief Get the last error as a string
 * TODO(Demi): Finish implementing this function
 * @return const char* 
 */
const char* ota_get_last_err_str() {  
    switch(last_err) { 
        case ESP_OK:
            return "OK"; 
        case ESP_ERR_INVALID_ARG:
            return "INVALID_ARG"; 
        case ESP_ERR_NO_MEM:
            return "NO MEMORY AVAILABLE";
        default: 
            return "AN UNKNOWN ERROR HAS OCCURED"; 
    }

}

/**
 * @brief Set the last error to ESP_OK
 * 
 */
void ota_clear_err() { 
    last_err = ESP_OK; 
}

/**
 * @brief Get the last error
 * 
 * @return esp_err_t 
 */
esp_err_t ota_get_last_err() { 
    return last_err; 
}