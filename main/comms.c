#include "comms.h"

#include "defines.h"

#include <string.h>
#include <stdbool.h>


bool comms_initialized = false; 

esp_err_t comms_init() { 
    //Initialize the communications transmit buffer
    com_buffer = (char*)malloc(sizeof(char) * TX_BUF_SIZE);
    memset(com_buffer, 0, sizeof(char) * TX_BUF_SIZE);

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

void comms_update_tx() { 
    if(com_buffer != NULL)
    {
        send_data(com_buffer);
        send_data("\r\n");
    }
}

void comms_update_rx(uint8_t *data) { 
    const int rx_bytes = uart_read_bytes(UART_CHANNEL, data, RX_BUF_SIZE, 100 / portTICK_PERIOD_MS);
    if (rx_bytes > 0) {
        data[rx_bytes] = 0;
        send_data((char*)data); 
    }
}

void copy_to_comms_buffer(char* data) {  
    const int len = strlen(data); 
    
    assert(len < TX_BUF_SIZE); 

    memccpy(com_buffer, data, 0, len); 
}

int send_data(char* data) { 
    if(!comms_initialized)
        return 0; 

    const int len = strlen(data);

    return send_data_with_length(data, len); 
}

int send_data_with_length(char* data, size_t len) {
    if(!comms_initialized)
        return 0; 

    return uart_write_bytes(UART_CHANNEL, data, len);
}

void clear_screen() { 
    send_data("\33[2J"); 
}