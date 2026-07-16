#include "PS4_BUS.h"
#include "main.h"

uint8_t PS4_ReadBus(void)
{
    uint8_t current_read = 0;
    uint8_t stable_read = 0;
    uint8_t match_count = 0;

    // Rapidly sample the pins up to 15 times
    for (int i = 0; i < 15; i++) {

        uint8_t bit0 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_12);
        uint8_t bit1 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11);
        uint8_t bit2 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);
        uint8_t bit3 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11);
        uint8_t bit4 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2);

        current_read = (bit4 << 4) | (bit3 << 3) | (bit2 << 2) | (bit1 << 1) | bit0;

        // If the reading matches the previous reading, increase our confidence
        if (current_read == stable_read) {
            match_count++;
        } else {
            // If it flickers, reset the confidence counter and track the new number
            stable_read = current_read;
            match_count = 1;
        }

        // If we see the EXACT same number 5 times in a row, the bus is perfectly stable!
        if (match_count >= 5) {
            return stable_read;
        }
    }

    // If the bus is too noisy and never stabilizes after 15 tries, safely return 0
    return 0;
}
