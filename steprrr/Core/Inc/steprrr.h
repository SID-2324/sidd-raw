/*
 * steprrr.h
 *
 *  Created on: Feb 22, 2026
 *      Author: LENOVO
 */
#ifndef INC_STEPRRR_H_
#define INC_STEPRRR_H_

#include "stm32f1xx_hal.h"

// --- Hardware Definitions ---
#define STEP_PIN   GPIO_PIN_5
#define STEP_PORT  GPIOB
#define DIR_PIN    GPIO_PIN_4
#define DIR_PORT   GPIOB

// --- Stepper Constants ---
#define MOTOR_STEPS_PER_REV 6400
#define MICROSTEPPING       32
#define GEAR_RATIO          19

// --- Prototypes ---
void Stepper_Init(TIM_HandleTypeDef *htim);
void Stepper_RotateAngle(float degrees, uint16_t speedUs, int direction);

#endif /* INC_STEPRRR_H_ */
