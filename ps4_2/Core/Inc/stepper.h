/*
 * stepper.h
 *
 *  Created on: Mar 10, 2026
 *      Author: LENOVO
 */

#ifndef INC_STEPPER_H_
#define INC_STEPPER_H_

#include "stm32f4xx_hal.h"
// --- Hardware Definitions ---
#define STEP_PIN   GPIO_PIN_8
#define STEP_PORT  GPIOC
#define DIR_PIN    GPIO_PIN_9
#define DIR_PORT   GPIOC

// --- Stepper Constants ---
#define MOTOR_STEPS_PER_REV 200
#define MICROSTEPPING       4
#define GEAR_RATIO          15

// --- Prototypes ---
void Stepper_Init(TIM_HandleTypeDef *htim);
void Stepper_RotateAngle(float degrees, uint16_t speedUs, int direction);


#endif /* INC_STEPPER_H_ */
