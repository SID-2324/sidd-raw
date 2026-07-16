#ifndef DMA_HANDLERS_H
#define DMA_HANDLERS_H

#include "stm32f4xx_hal.h"

// Expose the hardware flags so your step_servo.c can read them
extern volatile uint8_t base_tx_ready;
extern volatile uint8_t wrist_tx_ready;
extern volatile uint8_t delay_finished_flag;

#endif /* DMA_HANDLERS_H */
