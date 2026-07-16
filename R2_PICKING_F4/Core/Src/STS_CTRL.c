#include "STS_CTRL.h"
#include "STEP_SERVO.h"
#include "DMA_HANDLER.h"
/**
  * @brief Moves servo to a specific angle (Servo MUST be in Position Mode / Work Mode 0)
  * @param position: 0 to 4095
  * @param speed: 0 (max speed) to 3400
  */

uint8_t base_tx_buffer[13];
uint8_t wrist_tx_buffer[13];


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
    uint8_t *packet; // Pointer to point to the correct active array
    uint8_t checksum = 0;

    // 1. ROUTE AND LOCK THE CORRECT DMA PIPELINE
    if (huart->Instance == USART1)
    {
        while (base_tx_ready == 0) {} // Guard: Wait for previous Base DMA
        base_tx_ready = 0;            // Lock the Base pipeline
        packet = base_tx_buffer;      // Point to the Base array
    }
    else if (huart->Instance == USART2)
    {
        while (wrist_tx_ready == 0) {} // Guard: Wait for previous Wrist DMA
        wrist_tx_ready = 0;            // Lock the Wrist pipeline
        packet = wrist_tx_buffer;      // Point to the Wrist array
    }
    else
    {
        return; // Safety fallback if wrong UART is passed
    }

    // 2. BUILD THE PACKET DIRECTLY IN THE GLOBAL ARRAY
    packet[0] = 0xFF;
    packet[1] = 0xFF;
    packet[2] = id;
    packet[3] = 9;                   // Packet Length
    packet[4] = 0x03;                // Write Command
    packet[5] = STS_GOAL_POSITION_L; // Start writing at Position Low Byte

    packet[6] = position & 0xFF;
    packet[7] = (position >> 8) & 0xFF;
    packet[8] = 0;
    packet[9] = 0;
    packet[10] = speed & 0xFF;
    packet[11] = (speed >> 8) & 0xFF;

    // Calculate Checksum
    for (int i = 2; i < 12; i++) {
        checksum += packet[i];
    }
    packet[12] = ~checksum;

    // 3. FLIP PIN TO TRANSMIT AND FIRE DMA
    HAL_HalfDuplex_EnableTransmitter(huart);

    // Blast the 13 bytes in the background.
    // The CPU instantly moves to the next line without waiting!
    HAL_UART_Transmit_DMA(huart, packet, 13);

    // NOTE: HAL_HalfDuplex_EnableReceiver(huart) has been REMOVED from here
    // and safely placed inside dma_handlers.c
}

void STS_SyncWrite_Group(UART_HandleTypeDef *huart, uint8_t num_servos, uint8_t *ids, int16_t *positions, uint16_t *speeds)
{
    uint8_t *packet;
    uint8_t checksum = 0;

    // 1. ROUTE AND LOCK THE CORRECT DMA PIPELINE
    if (huart->Instance == USART1) {
        while (base_tx_ready == 0) {} // Wait for previous DMA to clear
        base_tx_ready = 0;
        packet = base_tx_buffer;
    }
    else if (huart->Instance == USART2) {
        while (wrist_tx_ready == 0) {}
        wrist_tx_ready = 0;
        packet = wrist_tx_buffer;
    }
    else {
        return;
    }

    // 2. CALCULATE PACKET DIMENSIONS
    // Math: (7 bytes per servo) + 4 bytes for the Instruction/Address header
    uint8_t packet_length = (num_servos * 7) + 4;
    uint8_t total_bytes = packet_length + 4; // Add FF FF FE and Length byte

    // 3. BUILD THE BROADCAST HEADER
    packet[0] = 0xFF;
    packet[1] = 0xFF;
    packet[2] = 0xFE;          // Broadcast ID (Everyone listens)
    packet[3] = packet_length;
    packet[4] = 0x83;          // 0x83 = SYNC WRITE Instruction
    packet[5] = 0x2A;          // Starting at Address 42 (Goal Position L)
    packet[6] = 0x06;          // Data length per servo (Pos L/H, Time L/H, Speed L/H)

    // 4. PACK THE DATA FOR EVERY SERVO IN THE ARRAYS
    uint8_t idx = 7;
    for (int i = 0; i < num_servos; i++)
    {
        packet[idx++] = ids[i];
        packet[idx++] = positions[i] & 0xFF;
        packet[idx++] = (positions[i] >> 8) & 0xFF;
        packet[idx++] = 0; // Time L (0 = use speed instead)
        packet[idx++] = 0; // Time H
        packet[idx++] = speeds[i] & 0xFF;
        packet[idx++] = (speeds[i] >> 8) & 0xFF;
    }

    // 5. CALCULATE CHECKSUM
    for (int i = 2; i < total_bytes - 1; i++) {
        checksum += packet[i];
    }
    packet[total_bytes - 1] = ~checksum;

    // 6. TRANSMIT VIA DMA
    HAL_HalfDuplex_EnableTransmitter(huart);
    HAL_UART_Transmit_DMA(huart, packet, total_bytes);
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
int16_t STS_ReadPosition(UART_HandleTypeDef *huart, uint8_t id)
{
    uint8_t tx_packet[8];
    uint8_t rx_packet[8];
    uint8_t checksum = 0;

    HAL_HalfDuplex_EnableTransmitter(huart);

    tx_packet[0] = 0xFF;
    tx_packet[1] = 0xFF;
    tx_packet[2] = id;
    tx_packet[3] = 4;
    tx_packet[4] = 0x02;
    tx_packet[5] = 0x38; // Present Position Address
    tx_packet[6] = 2;

    for (int i = 2; i < 7; i++) {
        checksum += tx_packet[i];
    }
    tx_packet[7] = ~checksum;

    HAL_UART_Transmit(huart, tx_packet, 8, 10);

    // CRITICAL: Wait for physical transmission to end
    while(__HAL_UART_GET_FLAG(huart, UART_FLAG_TC) == RESET);

    HAL_HalfDuplex_EnableReceiver(huart);

    // Timeout increased to 20ms for reliability at 1Mbps
    if (HAL_UART_Receive(huart, rx_packet, 8, 20) == HAL_OK) {
        if (rx_packet[0] == 0xFF && rx_packet[1] == 0xFF && rx_packet[2] == id) {
            return (int16_t)(rx_packet[6] << 8 | rx_packet[5]);
        }
    }

    return -1;
}


void STS_JogPositionUp(UART_HandleTypeDef *huart, uint8_t id, int16_t step_size, uint16_t speed)
{
    // 1. Read current position
    int16_t current_pos = STS_ReadPosition(huart, id);

    // 2. If reading was successful, calculate and write new position
    if (current_pos != -1) {
        int16_t new_pos = current_pos + step_size;

        // Prevent exceeding the hardware maximum limit
        if (new_pos > 4095) {
            new_pos = 4095;
        }

        STS_WritePosition(huart, id, new_pos, speed);
    }
}

void STS_JogPositionDown(UART_HandleTypeDef *huart, uint8_t id, int16_t step_size, uint16_t speed)
{
    // 1. Read current position
    int16_t current_pos = STS_ReadPosition(huart, id);

    // 2. If reading was successful, calculate and write new position
    if (current_pos != -1) {
        int16_t new_pos = current_pos - step_size;

        // Prevent dropping below the hardware minimum limit
        if (new_pos < 0) {
            new_pos = 0;
        }

        STS_WritePosition(huart, id, new_pos, speed);
    }
}

void STS_WritePosEx(UART_HandleTypeDef *huart, uint8_t id, int16_t position, uint16_t speed, uint8_t accel)
{
    uint8_t packet[14];
    uint8_t checksum = 0;

    HAL_HalfDuplex_EnableTransmitter(huart);

    packet[0] = 0xFF;
    packet[1] = 0xFF;
    packet[2] = id;
    packet[3] = 9;        // Length: 7 parameters + Instruction(1) + Address(1)
    packet[4] = 0x03;     // WRITE_DATA instruction

    // THE FIX: Acceleration starts at register 41 (0x29)
    packet[5] = 0x29;

    packet[6] = accel;                       // Register 0x29 (41): ACC
    packet[7] = position & 0xFF;             // Register 0x2A (42): Goal Pos L
    packet[8] = (position >> 8) & 0xFF;      // Register 0x2B (43): Goal Pos H
    packet[9] = 0;                           // Register 0x2C (44): Goal Time L
    packet[10] = 0;                          // Register 0x2D (45): Goal Time H
    packet[11] = speed & 0xFF;               // Register 0x2E (46): Goal Speed L
    packet[12] = (speed >> 8) & 0xFF;        // Register 0x2F (47): Goal Speed H

    // Checksum loop
    for (int i = 2; i < 13; i++) {
        checksum += packet[i];
    }
    packet[13] = ~checksum;

    HAL_UART_Transmit(huart, packet, 14, 100);

    // Note: HAL_UART_Transmit is blocking and already waits for TC,
    // but keeping this manual check is fine for safety.
    while(__HAL_UART_GET_FLAG(huart, UART_FLAG_TC) == RESET);

    HAL_HalfDuplex_EnableReceiver(huart);
}



void STS_StopMotor(UART_HandleTypeDef *huart, uint8_t id)
{
    STS_WriteMotorSpeed(huart, id, 0);
}
