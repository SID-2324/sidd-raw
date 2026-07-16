#include "vl53l1_platform.h"
#include "VL53L1X_api.h"
#include <stdio.h>
#include "PICKING_AUTONOMOUS.h"
extern I2C_HandleTypeDef hi2c1;
extern volatile uint8_t system_ready;
// ==============================================================
// 9-SENSOR UNIFIED GLOBAL CONFIGURATION
// ==============================================================
#define NUM_SENSORS 2

uint8_t sensor_status[NUM_SENSORS]  = {0};

// Sequential addresses for your array
uint16_t tof_addresses[NUM_SENSORS] = {
    0x54, 0x56
//	, 0x58, 0x5A, 0x5C, 0x5E, 0x60, 0x62, 0x64
};

// UPDATED WITH VALIDATED 9 XSHUT PORTS
GPIO_TypeDef* xshut_ports[NUM_SENSORS] = {
    GPIOB,  // TOF 1 (PB8)
    GPIOB,  // TOF 2 (PE1)
//    GPIOD,  // TOF 3 (PD0)
//    GPIOA,  // TOF 4 (PA15)
//    GPIOC,  // TOF 5 (PC9)
//    GPIOE,  // TOF 6 (PE8)
//    GPIOC,  // TOF 7 (PC4)
//    GPIOE,  // TOF 8 (PE7)
//    GPIOB   // TOF 9 (PB13)
};

// UPDATED WITH VALIDATED 9 XSHUT PINS
uint16_t xshut_pins[NUM_SENSORS] = {
    GPIO_PIN_9,   // TOF 1
    GPIO_PIN_8,   // TOF 2
//    GPIO_PIN_0,   // TOF 3
//    GPIO_PIN_15,  // TOF 4
//    GPIO_PIN_9,   // TOF 5
//    GPIO_PIN_8,   // TOF 6
//    GPIO_PIN_4,   // TOF 7
//    GPIO_PIN_7,   // TOF 8
//    GPIO_PIN_13   // TOF 9
};

#define VL53L1_I2C_TIMEOUT 100

// ==============================================================
// I2C HARDWARE WRAPPERS (STM32 HAL)
// ==============================================================
int8_t VL53L1_WriteMulti(uint16_t dev, uint16_t reg, uint8_t *pdata, uint32_t count)
{
    return HAL_I2C_Mem_Write(&hi2c1, dev, reg, I2C_MEMADD_SIZE_16BIT, pdata, count, VL53L1_I2C_TIMEOUT);
}

int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t reg, uint8_t *pdata, uint32_t count)
{
    return HAL_I2C_Mem_Read(&hi2c1, dev, reg, I2C_MEMADD_SIZE_16BIT, pdata, count, VL53L1_I2C_TIMEOUT);
}

int8_t VL53L1_WrByte(uint16_t dev, uint16_t reg, uint8_t data)
{
    return HAL_I2C_Mem_Write(&hi2c1, dev, reg, I2C_MEMADD_SIZE_16BIT, &data, 1, VL53L1_I2C_TIMEOUT);
}

int8_t VL53L1_WrWord(uint16_t dev, uint16_t reg, uint16_t data)
{
    uint8_t buf[2];
    buf[0] = data >> 8;
    buf[1] = data & 0xFF;
    return VL53L1_WriteMulti(dev, reg, buf, 2);
}

int8_t VL53L1_WrDWord(uint16_t dev, uint16_t reg, uint32_t data)
{
    uint8_t buf[4];
    buf[0] = (data >> 24) & 0xFF;
    buf[1] = (data >> 16) & 0xFF;
    buf[2] = (data >> 8) & 0xFF;
    buf[3] = data & 0xFF;
    return VL53L1_WriteMulti(dev, reg, buf, 4);
}

int8_t VL53L1_RdByte(uint16_t dev, uint16_t reg, uint8_t *data)
{
    return HAL_I2C_Mem_Read(&hi2c1, dev, reg, I2C_MEMADD_SIZE_16BIT, data, 1, VL53L1_I2C_TIMEOUT);
}

int8_t VL53L1_RdWord(uint16_t dev, uint16_t reg, uint16_t *data)
{
    uint8_t buf[2];
    uint8_t status = VL53L1_ReadMulti(dev, reg, buf, 2);
    *data = (buf[0] << 8) | buf[1];
    return status;
}

int8_t VL53L1_RdDWord(uint16_t dev, uint16_t reg, uint32_t *data)
{
    uint8_t buf[4];
    int8_t status = VL53L1_ReadMulti(dev, reg, buf, 4);
    *data = ((uint32_t)buf[0] << 24) |
            ((uint32_t)buf[1] << 16) |
            ((uint32_t)buf[2] << 8) |
            buf[3];
    return status;
}

int8_t VL53L1_WaitMs(uint16_t dev, int32_t wait_ms)
{
    HAL_Delay(wait_ms);
    return 0;
}

// ==============================================================
// CUSTOM HARDWARE WRAPPER: Get a stable, averaged distance
// ==============================================================
uint16_t VL53L1X_GetStableAverage(uint16_t dev, uint8_t num_samples)
{
    uint32_t sum_distance = 0;
    uint16_t temp_dist = 0;
    uint8_t ready = 0;
    uint32_t timeout_start = 0;

    for (uint8_t i = 0; i < num_samples; i++)
    {
        ready = 0;
        timeout_start = HAL_GetTick(); // Start the timer

        // Wait for fresh laser bounce WITH TIMEOUT
        while(ready == 0) {
            VL53L1X_CheckForDataReady(dev, &ready);

            // If the sensor hangs for more than 100ms, abort safely
            if (HAL_GetTick() - timeout_start > 100) {
                return 0; // Return 0 to trigger an [ABORT] in the main loop
            }
        }

        VL53L1X_GetDistance(dev, &temp_dist);
        VL53L1X_ClearInterrupt(dev);

        sum_distance += temp_dist;
    }

    return (uint16_t)(sum_distance / num_samples);
}

// ==============================================================
// I2C PHYSICAL BUS RECOVERY (The 9-Clock Unlock)
// ==============================================================
void I2C_Recover_Bus(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    printf(" -> [!] Executing Physical I2C Bus Unlock...\r\n");

    // 1. Disable the I2C peripheral completely
    __HAL_I2C_DISABLE(&hi2c1);

    // 2. Configure SCL (PB6) and SDA (PB7) as standard open-drain outputs
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 3. Set both high to start
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_SET);
    HAL_Delay(2);

    // 4. Toggle SCL 9 times to push the stuck sensor out of its jammed state
    for (int i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); // Clock Low
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);   // Clock High
        HAL_Delay(1);
    }

    // 5. Generate a manual I2C STOP condition
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); // SCL low
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET); // SDA low
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);   // SCL high
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);   // SDA high
    HAL_Delay(1);

    // 6. Return pins to I2C Alternate Function mode
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}


// ==============================================================
// 9-SENSOR SEQUENTIAL BOOT SEQUENCE
// ==============================================================
void VL53L1X_MultiBoot(void)
{

	// 1. SILENCE THE INTERRUPTS BEFORE BOOTING
	    HAL_NVIC_DisableIRQ(EXTI0_IRQn);
	    HAL_NVIC_DisableIRQ(EXTI1_IRQn);
	    HAL_NVIC_DisableIRQ(EXTI2_IRQn);
	    HAL_NVIC_DisableIRQ(EXTI3_IRQn);
	    HAL_NVIC_DisableIRQ(EXTI4_IRQn);
	    HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
	    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);

    printf("\r\n=== Starting Clean Array Boot ===\r\n");

    // 1. HARD WIPE: Turn OFF all sensors and reset status
    for (int i = 0; i < NUM_SENSORS; i++) {
        HAL_GPIO_WritePin(xshut_ports[i], xshut_pins[i], GPIO_PIN_RESET);
        sensor_status[i] = 0;
    }
    HAL_Delay(100);

    // 2. Clean the bus once at the very beginning
    __HAL_RCC_I2C1_FORCE_RESET();
    HAL_Delay(5);
    __HAL_RCC_I2C1_RELEASE_RESET();
    HAL_I2C_Init(&hi2c1);

    for (int i = 0; i < NUM_SENSORS; i++) {
        printf("\r\n--- Waking Sensor %d ---\r\n", i+1);

        // 1. WAKE THE SENSOR
        HAL_GPIO_WritePin(xshut_ports[i], xshut_pins[i], GPIO_PIN_SET);
        HAL_Delay(5); // Give it exactly 5ms to power up on the PCB

        // 2. THE SILVER BULLET: Hardware Ping
        // Do not attempt to read memory unless the hardware acknowledges the ping
        if (HAL_I2C_IsDeviceReady(&hi2c1, 0x52, 3, 50) != HAL_OK) {
            printf("[FATAL] Sensor %d did not ACK. Empty port or dead wire. Skipping.\r\n", i+1);

            // CRITICAL: Pull XSHUT back LOW so a broken sensor doesn't jam the 0x52 address for the next one!
            HAL_GPIO_WritePin(xshut_ports[i], xshut_pins[i], GPIO_PIN_RESET);
            continue;
        }

        printf(" -> Hardware ping success! Waiting for firmware boot...\r\n");

        // 3. HARDWARE IS ALIVE! Wait for firmware to boot
        uint8_t boot_state = 0;
        uint32_t boot_timeout = HAL_GetTick();

        while(boot_state == 0) {
            VL53L1X_BootState(0x52, &boot_state);
            HAL_Delay(2);
            if (HAL_GetTick() - boot_timeout > 100) break;
        }

        if (boot_state == 0) {
            printf("[FATAL] Sensor %d hardware alive but firmware hung. Skipping.\r\n", i+1);
            HAL_GPIO_WritePin(xshut_ports[i], xshut_pins[i], GPIO_PIN_RESET);
            continue;
        }

        printf(" -> Firmware loaded! Initializing to 0x%02X...\r\n", tof_addresses[i]);
        HAL_Delay(10); // Mandatory delay before heavy writes

        // 4. INITIALIZE AND CHANGE ADDRESS
        VL53L1X_SensorInit(0x52);
        HAL_Delay(5);
        VL53L1X_SetI2CAddress(0x52, tof_addresses[i]);
        HAL_Delay(10); // Let the new address settle on the bus

        // 5. Verify the sensor successfully moved to the new address
        if (HAL_I2C_IsDeviceReady(&hi2c1, tof_addresses[i], 3, 50) == HAL_OK) {
            sensor_status[i] = 1;
            printf("[SUCCESS] Sensor %d locked at 0x%02X.\r\n", i+1, tof_addresses[i]);
        } else {
            printf("[FATAL] Sensor %d failed to verify at new address.\r\n", i+1);
            HAL_GPIO_WritePin(xshut_ports[i], xshut_pins[i], GPIO_PIN_RESET);
        }
    }

    // STEP 4: CONFIGURE USING YOUR EXACT CUSTOM SETTINGS
    printf("\r\n--- Configuring Alive Sensors ---\r\n");
    for (int i = 0; i < NUM_SENSORS; i++) {
        if (sensor_status[i] == 1) {
            uint16_t dev = tof_addresses[i];

            VL53L1X_SetDistanceMode(dev, 1);
            VL53L1X_SetTimingBudgetInMs(dev, 50);
            VL53L1X_SetInterMeasurementInMs(dev, 60);
            VL53L1X_SetROI(dev, 6, 6);

            // EXTI CONFIGURATION: Mode 0 (Distance < 70mm triggers interrupt)
            VL53L1X_SetDistanceThreshold(dev, 70, 0, 0, 0);

            VL53L1X_StartRanging(dev);

            HAL_Delay(50);
            VL53L1X_ClearInterrupt(dev);

            printf("Sensor %d is ranging and EXTI Armed!\r\n", i+1);
        }
    }

    printf("=== Boot Complete ===\r\n\r\n");

    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |
                                 GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 |
                                 GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

    // 2. RE-ARM THE INTERRUPTS ONLY AFTER EVERYTHING IS STABLE
        HAL_NVIC_ClearPendingIRQ(EXTI0_IRQn);
        HAL_NVIC_EnableIRQ(EXTI0_IRQn);
        HAL_NVIC_ClearPendingIRQ(EXTI1_IRQn);
        HAL_NVIC_EnableIRQ(EXTI1_IRQn);
        HAL_NVIC_ClearPendingIRQ(EXTI2_IRQn);
        HAL_NVIC_EnableIRQ(EXTI2_IRQn);
        HAL_NVIC_ClearPendingIRQ(EXTI3_IRQn);
        HAL_NVIC_EnableIRQ(EXTI3_IRQn);
        HAL_NVIC_ClearPendingIRQ(EXTI4_IRQn);
        HAL_NVIC_EnableIRQ(EXTI4_IRQn);
        HAL_NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
        HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
        HAL_NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
        HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

        system_ready = 1;
}
void Scan_I2C_Bus(void) {
    printf("\r\n--- Starting Raw I2C Bus Scan ---\r\n");
    uint8_t devices_found = 0;

    // 1. Force TOF 1 Awake (GPIOB, PIN 8)
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_Delay(50); // 50ms boot time

    // 2. Ping every address
    for(uint16_t address = 1; address < 255; address++) {
        if (HAL_I2C_IsDeviceReady(&hi2c1, address, 3, 10) == HAL_OK) {
            printf(">>> SUCCESS: Device found listening at Address: 0x%X <<<\r\n", address);
            devices_found++;
        }
    }

    if (devices_found == 0) {
        printf("Scan Complete: Bus is totally empty. No hardware ACK received.\r\n");
    } else {
        printf("Scan Complete: %d device(s) found.\r\n", devices_found);
    }
    printf("-----------------------------------\r\n");
}
void Debug_Print_Sensor_Value(uint16_t dev_addr)
{
    uint8_t dataReady = 0;
    uint16_t distance = 0;

    printf("Pinging Sensor 0x%02X for snapshot...\r\n", dev_addr);

    // Clear any lingering interrupts before checking
    VL53L1X_ClearInterrupt(dev_addr);

    // Wait for the reading with a 500ms timeout
    uint32_t start_tick = HAL_GetTick();
    while (dataReady == 0)
    {
        VL53L1X_CheckForDataReady(dev_addr, &dataReady);

        // Anti-Freeze Protection
        if ((HAL_GetTick() - start_tick) > 500) {
            printf("[FAIL] Timeout! Sensor 0x%02X is dead or address is wrong.\r\n\r\n", dev_addr);
            return;
        }
    }

    // Data is ready! Pull it and clear.
    VL53L1X_GetDistance(dev_addr, &distance);
    VL53L1X_ClearInterrupt(dev_addr);

    printf("[SUCCESS] Sensor 0x%02X Distance: %d mm\r\n\r\n", dev_addr, distance);
}
