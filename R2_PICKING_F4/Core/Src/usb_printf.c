#include "usb_printf.h"
#include "usbd_cdc_if.h" // The STM32 USB CDC Middleware
#include <stdio.h>

/**
 * @brief  System Call Override for printf
 * @note   The standard C library (newlib) automatically routes all printf()
 * calls to this exact function name. We intercept it and push it over USB.
 */
int _write(int file, char *ptr, int len)
{
    // Transmit the string over the USB OTG cable
    CDC_Transmit_FS((uint8_t*)ptr, len);

    // Tiny delay to prevent USB buffer overflow when spamming printf in a while(1) loop
    HAL_Delay(1);

    return len;
}
