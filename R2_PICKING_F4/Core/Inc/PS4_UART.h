#ifndef PS4_UART_H
#define PS4_UART_H

#include "stm32f4xx_hal.h"

extern uint8_t ps4_btn;
extern int ps4_lx, ps4_ly, ps4_rx, ps4_ry;
extern uint8_t ps4_rx_buffer[25]; // DMA Buffer

// Added "volatile" to match the main.c and DMA_HANDLER.c files perfectly
extern volatile uint8_t ps4_packet_ready;

void PS4_ProcessPacket(void);

#endif /* PS4_UART_H */
