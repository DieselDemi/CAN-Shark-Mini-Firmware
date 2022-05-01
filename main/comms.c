#include "comms.h"

#include <string.h>
#include <stdbool.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <esp_log.h>

int send_string(char* data); 
int send_data_with_length(uint8_t* data, size_t len); 

bool comms_initialized = false; 

typedef struct comms_message_queue_t { 
    comms_message_t* messages; 
    size_t count; 
} comms_message_queue_t; 

comms_message_queue_t message_queue; 

/**
 * @brief Initialize the communications over USB via UART
 * 
 * @return esp_err_t ESP_OK if everything worked
 */
esp_err_t comms_init() { 
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
        
#ifdef COMMS_DEBUG
        for(size_t j = 0; j < message_queue.messages[i].data_length; j++) { 
            printf("%i ", message_queue.messages[i].data[j]);  
        }
        printf("\n");
#endif

        send_data_with_length(message_queue.messages[i].data, message_queue.messages[i].data_length); 
        send_string("\r\n");

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
void comms_update_rx(comms_status_t *status, uint8_t *data) { 
    const int rx_bytes = uart_read_bytes(UART_CHANNEL, data, RX_BUF_SIZE, 100 / portTICK_PERIOD_MS);
    if (rx_bytes > 0) {
        data[rx_bytes] = 0;

        if(*data == (uint8_t)'m') {
            status->sniff = true; 
            send_string("starting!");
        } 
        if(*data == (uint8_t)'n') {
            status->sniff = false; 
            send_string("stopping!");
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
comms_message_t create_message(comms_message_t* src, void* data, size_t len) { 
    uint8_t len_arr[4]; 
    len_arr[0] = (len >> 24) & 0xff; 
    len_arr[1] = (len >> 16) & 0xff; 
    len_arr[2] = (len >> 8) & 0xff; 
    len_arr[3] = (len) & 0xff; 

    src->data = (uint8_t*)malloc(sizeof(uint8_t) * len + 4); 
    
    memcpy(src->data, len_arr, 4); 
    memcpy(src->data + 4, data, len); 

    src->data_length = len + 4; 

    return *src; 
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

    return uart_write_bytes(UART_CHANNEL, data, len);
}

/**
 * @brief Clear the terminal screen
 * 
 */
void clear_screen() { 
    send_string("\33[2J"); 
}