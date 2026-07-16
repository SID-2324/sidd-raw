#ifndef STS_CTRL_H
#define STS_CTRL_H

#include "main.h"

// ST3215 Memory Addresses
#define STS_GOAL_POSITION_L 0x2A
#define STS_GOAL_TIME_L     0x2C
#define STS_GOAL_SPEED_L    0x2E
#define STS_TORQUE_ENABLE 0x28
// Function Prototypes
void STS_WritePosition(UART_HandleTypeDef *huart, uint8_t id, uint16_t position, uint16_t speed);
void STS_WriteMotorSpeed(UART_HandleTypeDef *huart, uint8_t id, int16_t speed);
void STS_WritePosEx(UART_HandleTypeDef *huart, uint8_t id, int16_t position, uint16_t speed, uint8_t accel);
void STS_StopMotor(UART_HandleTypeDef *huart, uint8_t id);
void STS_EnableTorque(UART_HandleTypeDef *huart, uint8_t id, uint8_t enable);
int16_t STS_ReadPosition(UART_HandleTypeDef *huart, uint8_t id);
#endif // STS_CTRL_H
