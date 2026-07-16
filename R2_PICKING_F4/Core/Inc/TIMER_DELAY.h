#ifndef TIMER_DELAYS_H
#define TIMER_DELAYS_H

#include "stm32f4xx_hal.h"

// Function prototypes
void Start_NonBlocking_Delay_us(uint16_t delay_us);
uint8_t Is_Delay_Finished(void);

#endif /* TIMER_DELAYS_H */
