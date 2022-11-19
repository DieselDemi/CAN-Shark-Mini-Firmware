#include "comms.h"

#include <string.h>
#include <stdbool.h>

#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/uart.h>
#include <sdkconfig.h>
#include <esp_intr_alloc.h>

#if CONFIG_IDF_TARGET_ESP32
    #include "esp32/rom/uart.h"
#elif CONFIG_IDF_TARGET_ESP32S2
    #include "esp32s2/rom/uart.h"
#endif

#include "ota.h"

int send_string(char* data); 
int send_formatted_data(uint8_t* data, size_t len); 
int send_data_with_length(uint8_t* data, size_t len); 

bool comms_initialized = false; 

typedef struct comms_message_queue_t { 
    comms_message_t* messages; 
    size_t count; 
} comms_message_queue_t; 

comms_message_queue_t message_queue; 

uint16_t calculate_crc16(uint8_t* data, size_t len);

/**
 * @brief Initialize the communications over USB via UART
 * 
 * @return esp_err_t ESP_OK if everything worked
 */
esp_err_t comms_init() { 

    esp_log_level_set("*", CONFIG_LOG_MAXIMUM_LEVEL);

    //Allocate the message queue
    message_queue.messages = (comms_message_t*)malloc(sizeof(comms_message_t) * MESSAGE_QUEUE_LEN); 
    memset(message_queue.messages, 0, sizeof(comms_message_t) * MESSAGE_QUEUE_LEN);
    message_queue.count = 0; 

    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    esp_err_t last_err = ESP_OK; 

    // We won't use a buffer for sending data.
    last_err = uart_driver_install(UART_CHANNEL, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    last_err = uart_param_config(UART_CHANNEL, &uart_config);
    last_err = uart_set_pin(UART_CHANNEL, UART_TXD_PIN, UART_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    comms_initialized = last_err == ESP_OK; 

    return last_err; 
}

/**
 * @brief Update method for the transmit task
 * 
 */
void comms_update_tx() { 
    for(size_t i = 0; i < message_queue.count; i++) {
        assert(send_string("<") == 1);
        send_formatted_data(message_queue.messages[i].data, message_queue.messages[i].data_length);
        assert(send_string(">\n") == 2);

        free(message_queue.messages[i].data); //Free the shit
    }

    //Clear the message queue 
    message_queue.count = 0; 
}

/**
 * @brief Update method for the recieve task
 * 
 * @param data 
 */
size_t update_progress_count = 0; 
size_t update_size = 0; 
uint8_t* update_buffer = NULL; 

void comms_update_rx(comms_status_t *status, char *data) {
    //If in update mode, we take all incoming data to do the update
    if(status->update) {     
        ESP_LOGD("COMMS - UPDATE", "Starting update"); 

        while(update_progress_count != update_size) {             
            int update_byte_count = uart_read_bytes(UART_CHANNEL, update_buffer, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS); 

            printf("Update count: %i\n", update_byte_count); 

            if(update_byte_count < 0) { 
                ESP_LOGE("COMMS - UPDATE", "Invalid data recieved"); 
                abort(); 
            } else if(update_byte_count > 0) { 
                ESP_ERROR_CHECK(ota_do_update(update_buffer, update_byte_count)); 
                            
                update_progress_count += update_byte_count;
                printf("Progress: %i of %i\n", update_progress_count, update_size);
                ESP_LOGD("COMMS - UPDATE", "Progress: %i of %i\n", update_progress_count, update_size);
            }
        } 

        status->update = false; 
        ota_cleanup();
        // send_data_with_length(update_data, update_size);
        send_string("Update Complete. Restarting...");

        return;
    }

    const int rx_bytes = uart_read_bytes(UART_CHANNEL, data, RX_BUF_SIZE, 100 / portTICK_PERIOD_MS);

    assert(rx_bytes <= RX_BUF_SIZE); 

    if (rx_bytes > 0) {
        // data[rx_bytes] = 0;
        ESP_LOGD("COMMS - UPDATE", "DATA INCOMING: %i\n", rx_bytes);

        if(strcmp(data, "m") == 0) {
            status->sniff = true; 
        } 
        if(strcmp(data, "n") == 0) {
            status->sniff = false; 
        }

        if(strcmp(data, "u") == 0) { 
            //Quick assertion that the buffer has atleast the correct amount of bytes
            assert(rx_bytes >= sizeof(size_t) + 1);

            memcpy(&update_size, data + 1, sizeof(size_t));                
            
            ESP_LOGD("COMMS - UPDATE", "Update - Size: %i\n", update_size); 

            uart_flush(UART_CHANNEL); 

            update_buffer = (uint8_t*)malloc(sizeof(uint8_t) * RX_BUF_SIZE); 

            if(update_buffer == NULL) { 
                ESP_LOGE("COMMS - UPDATE", "Could not allocate memory for update buffer"); 
                abort(); 
            }

            ESP_ERROR_CHECK(ota_init());

            status->update = true; 
        }
       
    }

}

/**
 * @brief Create a message object
 * 
 * @param src 
 * @param data 
 * @param len 
 * @return comms_message_t 
 */
esp_err_t create_message(comms_message_t* src, void* data, size_t len) { 

    uint16_t crc16 = calculate_crc16(data, len);
    uint8_t crc_arr[sizeof(uint16_t)]; 
    memcpy(crc_arr, &crc16, sizeof(uint16_t)); 

    len = len + sizeof(uint16_t);

    uint8_t len_arr[sizeof(size_t)]; 
    memcpy(len_arr, &len, sizeof(size_t));

    src->data = (uint8_t*)malloc(sizeof(uint8_t) * len + sizeof(size_t)); 
    
    memcpy(src->data, len_arr, sizeof(size_t)); 
    memcpy(src->data + sizeof(size_t) + 1, data, len - sizeof(uint16_t)); 
    memcpy(src->data + (len - sizeof(uint16_t)) + 1, crc_arr, sizeof(uint16_t)); 

    src->data_length = len + sizeof(size_t); 

    return ESP_OK; 
}

/**
 * @brief Add a message to the message queue
 * 
 * @param message message data
 */
void add_message(comms_message_t message) {
    if(message_queue.count > MESSAGE_QUEUE_LEN) {
        ESP_LOGE("COMMS", "MESSAGE QUEUE OVERRUN"); 
        return; 
    }

    message_queue.messages[message_queue.count++] = message; 
}

/**
 * @brief PRIVATE Send the data over uart
 * 
 * @param data 
 * @return int 
 */
int send_string(char* data) { 
    if(!comms_initialized)
        return 0; 

    const int len = strlen(data);

    return send_data_with_length((uint8_t*)data, len); 
}

// #define COMMS_DEBUG

/**
 * @brief PRIVATE Send data over uart
 * 
 * @param data
 * @param int
 */
int send_formatted_data(uint8_t* data, size_t len) { 
    //Reformat the data to ASCII
    char output_data[len * 4]; //Len * 4 because each aditional char is a 4 byte string
    memset(output_data, 0, len * 4); 

    int bytes_written = 0; 

    for(size_t i = 0; i < len; i++) { 
        char tmp_output[4];
        sprintf(tmp_output, "%02X", data[i]); 
        strcat(output_data, tmp_output); 
    }

    bytes_written = send_string(output_data); 

#ifdef COMMS_DEBUG
    printf("\n\nDEBUG: Len: %d Bytes Written: %d\n\n", len, bytes_written);
#endif

    return bytes_written;
}

// #undef COMMS_DEBUG

/**
 * @brief PRIVATE Send the data over uart with specified length
 * 
 * @param data 
 * @param len 
 * @return int 
 */
int send_data_with_length(uint8_t* data, size_t len) {
    if(!comms_initialized || len <= 0)
        return 0; 

#ifdef COMMS_DEBUG
    printf("Size: %d\n", len);
 
    for(size_t i = 0; i < len; i++) { 
        printf("%2x ", data[i]);  
    }
    printf("\n");
#endif

    return uart_write_bytes(UART_CHANNEL, data, len); 
}

/**
 * @brief Clear the terminal screen
 * 
 */
void clear_screen() { 
    send_string("\33[2J"); 
}

uint16_t crc_table[256] = { 
    0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
    0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7,
    0x0919, 0x1890, 0x2A0B, 0x3B82, 0x4F3D, 0x5EB4, 0x6C2F, 0x7DA6,
    0x8551, 0x94D8, 0xA643, 0xB7CA, 0xC375, 0xD2FC, 0xE067, 0xF1EE,
    0x1232, 0x03BB, 0x3120, 0x20A9, 0x5416, 0x459F, 0x7704, 0x668D,
    0x9E7A, 0x8FF3, 0xBD68, 0xACE1, 0xD85E, 0xC9D7, 0xFB4C, 0xEAC5,
    0x1B2B, 0x0AA2, 0x3839, 0x29B0, 0x5D0F, 0x4C86, 0x7E1D, 0x6F94,
    0x9763, 0x86EA, 0xB471, 0xA5F8, 0xD147, 0xC0CE, 0xF255, 0xE3DC,
    0x2464, 0x35ED, 0x0776, 0x16FF, 0x6240, 0x73C9, 0x4152, 0x50DB,
    0xA82C, 0xB9A5, 0x8B3E, 0x9AB7, 0xEE08, 0xFF81, 0xCD1A, 0xDC93,
    0x2D7D, 0x3CF4, 0x0E6F, 0x1FE6, 0x6B59, 0x7AD0, 0x484B, 0x59C2,
    0xA135, 0xB0BC, 0x8227, 0x93AE, 0xE711, 0xF698, 0xC403, 0xD58A,
    0x3656, 0x27DF, 0x1544, 0x04CD, 0x7072, 0x61FB, 0x5360, 0x42E9,
    0xBA1E, 0xAB97, 0x990C, 0x8885, 0xFC3A, 0xEDB3, 0xDF28, 0xCEA1,
    0x3F4F, 0x2EC6, 0x1C5D, 0x0DD4, 0x796B, 0x68E2, 0x5A79, 0x4BF0,
    0xB307, 0xA28E, 0x9015, 0x819C, 0xF523, 0xE4AA, 0xD631, 0xC7B8,
    0x48C8, 0x5941, 0x6BDA, 0x7A53, 0x0EEC, 0x1F65, 0x2DFE, 0x3C77,
    0xC480, 0xD509, 0xE792, 0xF61B, 0x82A4, 0x932D, 0xA1B6, 0xB03F,
    0x41D1, 0x5058, 0x62C3, 0x734A, 0x07F5, 0x167C, 0x24E7, 0x356E,
    0xCD99, 0xDC10, 0xEE8B, 0xFF02, 0x8BBD, 0x9A34, 0xA8AF, 0xB926,
    0x5AFA, 0x4B73, 0x79E8, 0x6861, 0x1CDE, 0x0D57, 0x3FCC, 0x2E45,
    0xD6B2, 0xC73B, 0xF5A0, 0xE429, 0x9096, 0x811F, 0xB384, 0xA20D,
    0x53E3, 0x426A, 0x70F1, 0x6178, 0x15C7, 0x044E, 0x36D5, 0x275C,
    0xDFAB, 0xCE22, 0xFCB9, 0xED30, 0x998F, 0x8806, 0xBA9D, 0xAB14,
    0x6CAC, 0x7D25, 0x4FBE, 0x5E37, 0x2A88, 0x3B01, 0x099A, 0x1813,
    0xE0E4, 0xF16D, 0xC3F6, 0xD27F, 0xA6C0, 0xB749, 0x85D2, 0x945B,
    0x65B5, 0x743C, 0x46A7, 0x572E, 0x2391, 0x3218, 0x0083, 0x110A,
    0xE9FD, 0xF874, 0xCAEF, 0xDB66, 0xAFD9, 0xBE50, 0x8CCB, 0x9D42,
    0x7E9E, 0x6F17, 0x5D8C, 0x4C05, 0x38BA, 0x2933, 0x1BA8, 0x0A21,
    0xF2D6, 0xE35F, 0xD1C4, 0xC04D, 0xB4F2, 0xA57B, 0x97E0, 0x8669,
    0x7787, 0x660E, 0x5495, 0x451C, 0x31A3, 0x202A, 0x12B1, 0x0338,
    0xFBCF, 0xEA46, 0xD8DD, 0xC954, 0xBDEB, 0xAC62, 0x9EF9, 0x8F70
};

uint16_t calculate_crc16(uint8_t* data, size_t len){ 
    uint16_t ret = 0xffff; 

    for(int i = 0; i < len; i++) { 
        int lui = (ret ^ *(data + i)) & 0xff; 
        ret = (ret >> 8) ^ crc_table[lui]; 
    }

    return ret ^ 0xffff; 
}