#include "STS_CTRL.h"

/**
  * @brief Moves servo to a specific angle (Servo MUST be in Position Mode / Work Mode 0)
  * @param position: 0 to 4095
  * @param speed: 0 (max speed) to 3400
  */
void STS_EnableTorque(UART_HandleTypeDef *huart, uint8_t id, uint8_t enable)
{
    uint8_t packet[8];
    uint8_t checksum = 0;

    HAL_HalfDuplex_EnableTransmitter(huart);

    packet[0] = 0xFF;
    packet[1] = 0xFF;
    packet[2] = id;
    packet[3] = 4;                 // Packet Length
    packet[4] = 0x03;              // Write Command
    packet[5] = STS_TORQUE_ENABLE; // Register Address
    packet[6] = enable;            // 1 or 0

    // Calculate Checksum
    for (int i = 2; i < 7; i++) {
        checksum += packet[i];
    }
    packet[7] = ~checksum;

    HAL_UART_Transmit(huart, packet, 8, 100);
    HAL_HalfDuplex_EnableReceiver(huart);
}


void STS_WritePosition(UART_HandleTypeDef *huart, uint8_t id, uint16_t position, uint16_t speed)
{
    uint8_t packet[13];
    uint8_t checksum = 0;

    // Force pin to transmit
    HAL_HalfDuplex_EnableTransmitter(huart);

    packet[0] = 0xFF;
    packet[1] = 0xFF;
    packet[2] = id;
    packet[3] = 9;                 // Packet Length
    packet[4] = 0x03;              // Write Command
    packet[5] = STS_GOAL_POSITION_L; // Start writing at Position Low Byte

    packet[6] = position & 0xFF;         // Position Low
    packet[7] = (position >> 8) & 0xFF;  // Position High
    packet[8] = 0;                       // Time Low (0 = use speed parameter instead)
    packet[9] = 0;                       // Time High
    packet[10] = speed & 0xFF;           // Speed Low
    packet[11] = (speed >> 8) & 0xFF;    // Speed High

    // Calculate Checksum
    for (int i = 2; i < 12; i++) {
        checksum += packet[i];
    }
    packet[12] = ~checksum;

    HAL_UART_Transmit(huart, packet, 13, 100);

    // Return pin to receive mode
    HAL_HalfDuplex_EnableReceiver(huart);
}

/**
  * @brief Spins servo continuously (Servo MUST be in Motor Mode / Work Mode 1)
  * @param speed: Positive values (e.g., 1000) for forward, Negative values (e.g., -1000) for reverse. Max is 32767.
  */
void STS_WriteMotorSpeed(UART_HandleTypeDef *huart, uint8_t id, int16_t speed)
{
    uint8_t packet[9];
    uint8_t checksum = 0;
    uint16_t speed_val;

    // Bit 15 controls direction in motor mode. 0-14 controls speed.
    if (speed < 0) {
        speed_val = (uint16_t)(-speed) | 0x8000;
    } else {
        speed_val = (uint16_t)speed;
    }

    HAL_HalfDuplex_EnableTransmitter(huart);

    packet[0] = 0xFF;
    packet[1] = 0xFF;
    packet[2] = id;
    packet[3] = 5;               // Packet Length
    packet[4] = 0x03;            // Write Command
    packet[5] = STS_GOAL_SPEED_L; // Start writing at Speed Low Byte

    packet[6] = speed_val & 0xFF;
    packet[7] = (speed_val >> 8) & 0xFF;

    // Calculate Checksum
    for (int i = 2; i < 8; i++) {
        checksum += packet[i];
    }
    packet[8] = ~checksum;

    HAL_UART_Transmit(huart, packet, 9, 100);
    HAL_HalfDuplex_EnableReceiver(huart);
}

/**
  * @brief Instantly stops the servo when in Motor Mode
  */
void STS_StopMotor(UART_HandleTypeDef *huart, uint8_t id)
{
    STS_WriteMotorSpeed(huart, id, 0);
}
