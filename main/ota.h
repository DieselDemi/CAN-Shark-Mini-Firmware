#ifndef _OTA_H_
#define _OTA_H_

#include "esp_check.h"

esp_err_t ota_init(); 
esp_err_t ota_do_update(void* image, size_t img_size); 
void ota_cleanup(); 
esp_err_t ota_do_after_update(); 

esp_err_t ota_get_last_err(); 
const char* ota_get_last_err_str(); 
void ota_clear_err(); 

#endif
