#ifndef ESP_RECEIVER_H
#define ESP_RECEIVER_H

#include "main.h"

// Define readable names for your commands
#define CMD_IDLE       0
#define CMD_BACKWARD    1
#define CMD_FORWARD    2
#define CMD_LEFT       3
#define CMD_RIGHT      4
#define CMD_CW         5
#define CMD_CCW        6

// Function Prototype
void ESP_ProcessCommand(UART_HandleTypeDef *huart);

#endif /* ESP_RECEIVER_H */
