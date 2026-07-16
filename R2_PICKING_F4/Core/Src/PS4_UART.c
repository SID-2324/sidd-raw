#include "PS4_UART.h"
#include <stdio.h>
#include <string.h>

// The DMA hardware will constantly overwrite this array in the background
uint8_t ps4_rx_buffer[25];

// Pull in the UART handle from main.c so we can restart DMA if needed
extern UART_HandleTypeDef huart3;

// Global variables your locomotion math will use
uint8_t ps4_btn = 0;
int ps4_lx = 0, ps4_ly = 0, ps4_rx = 0, ps4_ry = 0;

#define JOYSTICK_DEADBAND 20

static int ApplyDeadband(int axis_value) {
    if (axis_value > -JOYSTICK_DEADBAND && axis_value < JOYSTICK_DEADBAND) {
        return 0;
    }
    return axis_value;
}

/**
 * @brief Call this function inside your while(1) loop before running locomotion math.
 * It directly reads the DMA memory. No interrupts required.
 */
void PS4_ProcessPacket(void)
{
    // 1. THE AUTO-ALIGNMENT GUARD
    // If the packet does not start with '<' or end with '\n', the DMA is misaligned.
    if (ps4_rx_buffer[0] != '<' || ps4_rx_buffer[24] != '\n')
    {
        // Kill the misaligned DMA stream and restart it to catch the next packet cleanly
        HAL_UART_AbortReceive(&huart3);
        HAL_UART_Receive_DMA(&huart3, ps4_rx_buffer, 25);
        return; // Skip parsing this cycle to prevent crazy joystick values
    }

    // 2. PARSE THE SECURE DATA
    int temp_btn, lx, ly, rx, ry;

    // Copy the raw bytes to a temporary string and add a null terminator
    // This makes sscanf 100% crash-proof
    char safe_string[26];
    memcpy(safe_string, ps4_rx_buffer, 25);
    safe_string[25] = '\0';

    if (sscanf(safe_string, "<%d,%d,%d,%d,%d>", &temp_btn, &lx, &ly, &rx, &ry) == 5)
    {
        ps4_btn = (uint8_t)temp_btn;
        ps4_lx = ApplyDeadband(lx);
        ps4_ly = ApplyDeadband(ly);
        ps4_rx = ApplyDeadband(rx);
        ps4_ry = ApplyDeadband(ry);
    }
}
