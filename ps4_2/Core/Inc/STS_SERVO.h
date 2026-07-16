/*
 * STS_SERVO.h
 *
 *  Created on: Mar 10, 2026
 *      Author: LENOVO
 */

#ifndef INC_STS_SERVO_H_
#define INC_STS_SERVO_H_


#include "main.h"

// ===== BASIC COMMANDS =====
void STS_EnableTorque(UART_HandleTypeDef *huart, uint8_t id);
void STS_WritePos(UART_HandleTypeDef *huart, uint8_t id, uint16_t pos, uint16_t speed);
void STS_SetAccel(UART_HandleTypeDef *huart, uint8_t id, uint8_t accel);

// ===== SMART MOVE FUNCTION =====
void moveSmooth(UART_HandleTypeDef *huart, uint8_t id, uint16_t pos, uint16_t speed, uint8_t accel);

#endif

/* INC_STS_SERVO_H_ */
