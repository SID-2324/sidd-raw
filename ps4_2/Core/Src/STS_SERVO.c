#include "STS_SERVO.h"

// Enable torque
void STS_EnableTorque(UART_HandleTypeDef *huart, uint8_t id)
{
    uint8_t packet[8];
    uint8_t checksum = 0;

    packet[0]=0xFF; packet[1]=0xFF;
    packet[2]=id;
    packet[3]=4;
    packet[4]=0x03;
    packet[5]=0x28;
    packet[6]=1;

    for(int i=2;i<7;i++) checksum+=packet[i];
    checksum=~checksum;

    HAL_UART_Transmit(huart,packet,7,100);
    HAL_UART_Transmit(huart,&checksum,1,100);
}


// Write position + speed
void STS_WritePos(UART_HandleTypeDef *huart, uint8_t id, uint16_t pos, uint16_t speed)
{
    uint8_t packet[10];
    uint8_t checksum = 0;

    packet[0]=0xFF;
    packet[1]=0xFF;
    packet[2]=id;
    packet[3]=7;
    packet[4]=0x03;
    packet[5]=0x2A;

    packet[6]=pos & 0xFF;
    packet[7]=pos >> 8;
    packet[8]=speed & 0xFF;
    packet[9]=speed >> 8;

    for(int i=2;i<10;i++) checksum+=packet[i];
    checksum=~checksum;

    HAL_UART_Transmit(huart,packet,10,100);
    HAL_UART_Transmit(huart,&checksum,1,100);
}


// Set acceleration
void STS_SetAccel(UART_HandleTypeDef *huart, uint8_t id, uint8_t accel)
{
    uint8_t packet[8];
    uint8_t checksum = 0;

    packet[0]=0xFF; packet[1]=0xFF;
    packet[2]=id;
    packet[3]=4;
    packet[4]=0x03;
    packet[5]=0x29;
    packet[6]=accel;

    for(int i=2;i<7;i++) checksum+=packet[i];
    checksum=~checksum;

    HAL_UART_Transmit(huart,packet,7,100);
    HAL_UART_Transmit(huart,&checksum,1,100);
}


// Smooth move (speed + accel)
void moveSmooth(UART_HandleTypeDef *huart, uint8_t id, uint16_t pos, uint16_t speed, uint8_t accel)
{
    STS_SetAccel(huart, id, accel);
    HAL_Delay(20);
    STS_WritePos(huart, id, pos, speed);
}
