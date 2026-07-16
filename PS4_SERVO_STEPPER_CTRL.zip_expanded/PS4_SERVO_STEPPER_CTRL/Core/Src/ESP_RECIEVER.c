#include "ESP_RECIEVER.h"
#include "STS_CTRL.h"
#include <stdio.h>
#include <string.h>

// 1. INCLUDE YOUR SERVO HEADER HERE
// #include "st3215_servo.h"

static uint8_t last_rx_data = 255;
//static uint16_t servo_pos = 0;
//static uint8_t  servo_id_1 = 3;   // YOUR SERVO ID
//static uint8_t  servo_id_2 = 7;
extern UART_HandleTypeDef huart1;
void ESP_ProcessCommand(UART_HandleTypeDef *huart)
{
    uint8_t rx_data = 0;
    char msg[80];

    // Read the 3 input pins from ESP32
    uint8_t bit0 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8);
    uint8_t bit1 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9);
    uint8_t bit2 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10);

    // Reconstruct the integer command (0 to 6)
    rx_data = (bit2 << 2) | (bit1 << 1) | bit0;

    // --- THE INTEGRATION ZONE ---
    // Only execute when the PS4 button state actually changes
    if (rx_data != last_rx_data) {

        switch(rx_data) {
            case CMD_IDLE:
                sprintf(msg, "STM32: IDLE (Stopping Servos)\r\n");
                // Example: Stop all continuous rotation servos and stepper
                STS_StopMotor(&huart1, 1);
                break;

            case CMD_BACKWARD:
                sprintf(msg, "STM32: STEPPER 180 (Cross)\r\n");
                Stepper_RotateAngle(90.0, 500, 1);
                break;

            case CMD_FORWARD:
                sprintf(msg, "STM32: STEPPER 90 (Triangle)\r\n");
                Stepper_RotateAngle(90.0, 500, 1);
                break;

            case CMD_LEFT:
                sprintf(msg, "STM32: SERVO (Square)\r\n");
                STS_WriteMotorSpeed(&huart1, 1, -1500);
                break;

            case CMD_RIGHT:
                sprintf(msg, "STM32: RIGHT (Circle)\r\n");
                STS_WriteMotorSpeed(&huart1, 1, 1500);
                break;

            case CMD_CW:
                sprintf(msg, "STM32: Clockwise (R2)\r\n");
                // Add your servo logic here

                break;

            case CMD_CCW:
                sprintf(msg, "STM32: Anti-clockwise (L2)\r\n");
                // Add your servo logic here

                break;

            default:
                sprintf(msg, "STM32: Unknown Command\r\n");
                break;
        }

        // Print the status to the PC
        HAL_UART_Transmit(huart, (uint8_t*)msg, strlen(msg), 100);

        last_rx_data = rx_data; // Remember this state to prevent bus spam
    }

    // Note: I removed the old GPIO motor toggles (PA5, PA6, etc.)
    // since you are upgrading to serial bus servos!
}
