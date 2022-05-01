#include <esp_system.h>
#define CAN_BUS_TX_GPIO_NUM  GPIO_NUM_22
#define CAN_BUS_RX_GPIO_NUM  GPIO_NUM_21
#define UART_TXD_PIN GPIO_NUM_1
#define UART_RXD_PIN GPIO_NUM_3
#define UART_CHANNEL UART_NUM_0

#define RX_BUF_SIZE 1024
#define TX_BUF_SIZE 128 //TODO(Demi): Calculate the maximum message size to be sent to the host pc
#define MESSAGE_QUEUE_LEN 2048

#define CAN_TICKS_TO_WAIT 100

#define CAN_DEBUG
// #define COMMS_DEBUG