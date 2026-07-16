#ifndef __STEP_SERVO_H
#define __STEP_SERVO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

// MANUAL MODE: Executes joystick commands
void STS_JogCommand(UART_HandleTypeDef *huart_base, UART_HandleTypeDef *huart_wrist, uint8_t rx_data);
// Add this below your other function prototypes
void STS_SyncWrite_Group(UART_HandleTypeDef *huart, uint8_t num_servos, uint8_t *ids, int16_t *positions, uint16_t *speeds);
void STS_CalibrateEnvironment();
// AUTO MODE: Executes the 157mm kinematic math
void STS_AutoPickCommand(UART_HandleTypeDef *huart_base, UART_HandleTypeDef *huart_wrist, uint8_t ps4_button);
void SERVO_BOOT(UART_HandleTypeDef *huart_base, UART_HandleTypeDef *huart_wrist);

#ifdef __cplusplus
}
#endif

#endif /* __STEP_SERVO_H */
