#ifndef PLATFORM_H
#define PLATFORM_H

#include "main.h"

extern I2C_HandleTypeDef hi2c1;  // or your I2C handle

int8_t VL53L1_WriteMulti(uint16_t dev, uint16_t reg, uint8_t *pdata, uint32_t count);
int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t reg, uint8_t *pdata, uint32_t count);
int8_t VL53L1_WrByte(uint16_t dev, uint16_t reg, uint8_t data);
int8_t VL53L1_WrWord(uint16_t dev, uint16_t reg, uint16_t data);
int8_t VL53L1_WrDWord(uint16_t dev, uint16_t reg, uint32_t data);
int8_t VL53L1_RdByte(uint16_t dev, uint16_t reg, uint8_t *data);
int8_t VL53L1_RdWord(uint16_t dev, uint16_t reg, uint16_t *data);
int8_t VL53L1_RdDWord(uint16_t dev, uint16_t reg, uint32_t *data);
int8_t VL53L1_WaitMs(uint16_t dev, int32_t wait_ms);
uint16_t VL53L1X_GetStableAverage(uint16_t dev, uint8_t num_samples);
void Debug_Print_Sensor_Value(uint16_t dev_addr);
// Add this right below VL53L1X_GetStableAverage
//void VL53L1X_MultiBoot(uint16_t *dev_array);
void VL53L1X_MultiBoot(void);
uint16_t VL53L1X_HardwareBoot(void);
#endif
