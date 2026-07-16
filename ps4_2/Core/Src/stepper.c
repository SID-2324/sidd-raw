/*
 * steprrr.c
 *
 *  Created on: Feb 22, 2026
 *      Author: LENOVO
 */
#include "stepper.h"
#include <stdio.h>

static TIM_HandleTypeDef *stepper_htim;
static float STEPS_PER_DEGREE;

void Stepper_Init(TIM_HandleTypeDef *htim) {
    stepper_htim = htim;
    // Start the timer so microDelay doesn't hang the system
    HAL_TIM_Base_Start(stepper_htim);
    STEPS_PER_DEGREE = (float)(MOTOR_STEPS_PER_REV * MICROSTEPPING * GEAR_RATIO) / 360.0f;
}

static void microDelay(uint16_t delay) {
    __HAL_TIM_SET_COUNTER(stepper_htim, 0);
    // Wait for the counter to reach the delay value
    while (__HAL_TIM_GET_COUNTER(stepper_htim) < delay);
}

void Stepper_RotateAngle(float degrees, uint16_t speedUs, int direction) {
    long stepsToTake = (long)(degrees * STEPS_PER_DEGREE);

    // Set Direction
    HAL_GPIO_WritePin(DIR_PORT, DIR_PIN, (direction == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    for(long x = 0; x < stepsToTake; x++) {
        HAL_GPIO_WritePin(STEP_PORT, STEP_PIN, GPIO_PIN_SET);
        microDelay(speedUs);
        HAL_GPIO_WritePin(STEP_PORT, STEP_PIN, GPIO_PIN_RESET);
        microDelay(speedUs);
    }
}
