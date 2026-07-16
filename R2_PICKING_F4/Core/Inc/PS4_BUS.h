#ifndef __PS4_BUS_H
#define __PS4_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

// Reads the 5 hardware pins and returns the 8-bit command
uint8_t PS4_ReadBus(void);

#ifdef __cplusplus
}
#endif

#endif /* __PS4_BUS_H */
