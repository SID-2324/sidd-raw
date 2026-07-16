#include "PICKING_AUTONOMOUS.h"
#include "VL53L1X_api.h"
#include "vl53l1_platform.h"
#include "VL53L1X_calibration.h"
#include <string.h>
#include "STS_CTRL.h"
#include "TIMER_DELAY.h"
#include "PS4_UART.h"
#include <stdio.h>
// The addresses of your 9 sensors
static const uint16_t tof_addresses[NUM_SENSORS] = {
    0x54, 0x56,
//	0x58, 0x5A, 0x5C, 0x5E, 0x60, 0x62, 0x64
};

// The global map of the robot's surroundings
uint16_t live_sensor_distances[NUM_SENSORS] = {0};
volatile uint8_t system_ready = 0;
volatile uint16_t active_sensor_exti = 0; // Holds the I2C address of the ONLY sensor allowed to trigger
volatile uint8_t exti_triggered = 0;     // The flag the while loop watches

volatile uint8_t new_cmd_flag = 0;
volatile uint8_t active_cmd = '0';
uint8_t Check_Emergency_Stop(void);

void TOF_InitArray(void) {
    for(int i = 0; i < NUM_SENSORS; i++) {
        VL53L1X_ClearInterrupt(tof_addresses[i]);
    }
}

// Call this constantly in your while(1) loop
void TOF_PollContinuous(void) {
    for (int i = 0; i < NUM_SENSORS; i++) {
        uint8_t dataReady = 0;
        VL53L1X_CheckForDataReady(tof_addresses[i], &dataReady);

        if (dataReady) {
            // Update the global map
            VL53L1X_GetDistance(tof_addresses[i], &live_sensor_distances[i]);

            // Clear the interrupt so the sensor takes a new measurement
            VL53L1X_ClearInterrupt(tof_addresses[i]);
        }
    }
}

// Helper to map an EXTI pin to your I2C addresses
uint16_t Map_EXTI_Pin_To_Address(uint16_t GPIO_Pin) {
    // Mapped based on the EXTI pins visible in image_0919a0.png
    //if (GPIO_Pin == GPIO_PIN_0)  return 0x54; // PE0  (GPIO_EXTI0)
    if (GPIO_Pin == GPIO_PIN_9)  return 0x56; // PB9  (GPIO_EXTI1)
    if (GPIO_Pin == GPIO_PIN_2)  return 0x58; // PD2  (GPIO_EXTI2)
    if (GPIO_Pin == GPIO_PIN_11)  return 0x5A; // PC11  (GPIO_EXTI3)
    if (GPIO_Pin == GPIO_PIN_3)  return 0x5C; // PD3  (GPIO_EXTI4)
    if (GPIO_Pin == GPIO_PIN_4)  return 0x5E; // PA4  (GPIO_EXTI9)
    if (GPIO_Pin == GPIO_PIN_0) return 0x60; // PB0 (GPIO_EXTI11)
    if (GPIO_Pin == GPIO_PIN_1) return 0x62; // PB1 (GPIO_EXTI14)
    //if (GPIO_Pin == GPIO_PIN_1) return 0x62; // PB1 (GPIO_EXTI14)
    // TODO: Add your 9th sensor once configured in CubeMX
    // if (GPIO_Pin == GPIO_PIN_X) return 0x64;

    return 0; // Unknown pin
}

// The Universal STM32 EXTI Callback
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // 1. SAFETY GATE: If the system is still booting or configuring, IGNORE!
    if (system_ready == 0) return;

    // 2. RAW HARDWARE TEST
    // Use a flag for the main loop, keep the callback as fast as possible
    uint16_t triggered_dev = Map_EXTI_Pin_To_Address(GPIO_Pin);

    // 3. The Gate
    if (triggered_dev != 0 && triggered_dev == active_sensor_exti) {
        exti_triggered = 1;
        // OPTIONAL: Move the printf to the while loop to prevent I2C bus collision
    }
}

void Servo_PickSpearhead(UART_HandleTypeDef *huart_base, UART_HandleTypeDef *huart_wrist)


{
	    uint16_t speed = 3000;
	    uint16_t DISTANCE_SPEARHEAD_CLIMB_RACK_1 = 9260;
	    uint16_t chunk_forward_1  = DISTANCE_SPEARHEAD_CLIMB_RACK_1;          // 100
	    uint16_t chunk_backward_1 = DISTANCE_SPEARHEAD_CLIMB_RACK_1 | 0x8000; // 32868
        // Blast Pair 1 (Base + Wrist simultaneously via DMA)
        STS_WritePosition(huart_base, 120, chunk_forward_1, speed);
        STS_WritePosition(huart_wrist, 124, chunk_backward_1, speed);
        Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());

        // Blast Pair 2
        STS_WritePosition(huart_base, 121, chunk_backward_1, speed);
        STS_WritePosition(huart_wrist, 125, chunk_forward_1, speed);
        Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());

        // Blast Pair 3
        STS_WritePosition(huart_base, 122, chunk_forward_1, speed);
        STS_WritePosition(huart_wrist, 126, chunk_backward_1, speed);
        Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());

        // Blast Pair 4
        STS_WritePosition(huart_base, 123, chunk_backward_1, speed);
        STS_WritePosition(huart_wrist, 127, chunk_forward_1, speed);
    }
void Macro_PickSpearhead(UART_HandleTypeDef *huart_base, UART_HandleTypeDef *huart_wrist)
{
    uint16_t spearhead_distance = 2000;
    uint16_t spearhead_rack_distance = 2000;
    uint8_t rangeStatus = 255;
    int32_t auto_speed = -6500;
    uint8_t error_count = 0;
//    uint16_t speed = 3000;
//    uint16_t DISTANCE_SPEARHEAD_CLIMB_RACK_1 = 9260;
//    uint16_t chunk_forward_1  = DISTANCE_SPEARHEAD_CLIMB_RACK_1;          // 100
//    uint16_t chunk_backward_1 = DISTANCE_SPEARHEAD_CLIMB_RACK_1 | 0x8000; // 32868

    printf("\r\n=== STARTING MACRO: PICK SPEARHEAD (ROBUST POLLING) ===\r\n");
    printf("PHASE 1: Actuating Servos...\r\n");
//    {
//        // Blast Pair 1 (Base + Wrist simultaneously via DMA)
//        STS_WritePosition(huart_base, 120, chunk_forward_1, speed);
//        STS_WritePosition(huart_wrist, 124, chunk_backward_1, speed);
//        Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
//
//        // Blast Pair 2
//        STS_WritePosition(huart_base, 121, chunk_backward_1, speed);
//        STS_WritePosition(huart_wrist, 125, chunk_forward_1, speed);
//        Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
//
//        // Blast Pair 3
//        STS_WritePosition(huart_base, 122, chunk_forward_1, speed);
//        STS_WritePosition(huart_wrist, 126, chunk_backward_1, speed);
//        Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
//
//        // Blast Pair 4
//        STS_WritePosition(huart_base, 123, chunk_backward_1, speed);
//        STS_WritePosition(huart_wrist, 127, chunk_forward_1, speed);
//    }

    // 1. Reset sensor state
    VL53L1X_StopRanging(TOF_RACK_FRONT);
    VL53L1X_StartRanging(TOF_RACK_FRONT);
    //Start_NonBlocking_Delay_us(5000000); while(!Is_Delay_Finished());
    HAL_Delay(5000);

    // 2. THE ROBUST APPROACH LOOP
    while (spearhead_rack_distance > 100)
    {
        if (Check_Emergency_Stop()) return;

        // Drive mecanum forward
        Mechanum_DriveStraight(auto_speed);

        // Fetch status and distance
        VL53L1X_GetRangeStatus(TOF_RACK_FRONT, &rangeStatus);

        if (rangeStatus == 0) {
            uint16_t raw_dist = 0;
            VL53L1X_GetDistance(TOF_RACK_FRONT, &raw_dist);

            // Validate: ignore impossible readings
            if (raw_dist < 3000) {
                spearhead_distance = raw_dist;
                error_count = 0; // Reset error on valid read
            }
        } else {
            error_count++;
            // Safety: Stop if sensor consistently fails to return valid data
            if (error_count > 5) {
                printf("!!! SENSOR ERROR: Shutting down motors !!!\r\n");
                Mechanum_DriveStraight(0);
                return;
            }
        }

        printf("Front Dist: %d mm | Status: %d\r\n", spearhead_distance, rangeStatus);
        HAL_Delay(25);
    }
}

uint8_t Check_Emergency_Stop(void) {
    if (active_cmd == 'Z') {
        active_cmd = 0;                          // clear it so it doesn't retrigger
        Mechanum_DriveStraight(0);               // kill motors immediately
        printf("\r\n!!! EMERGENCY STOP TRIGGERED (USB) !!!\r\n");
        return 1;
    }
    return 0;
}

// --- COMPLETE CLIMBING MACRO ---
void Macro_Climb_Plus20(UART_HandleTypeDef *huart_base, UART_HandleTypeDef *huart_wrist)
{
    uint16_t dist_front = 2000;
    uint16_t dist_rear = 2000;
    int32_t auto_speed = 5000;
    uint8_t rangeStatus = 255;
    uint8_t error_count = 0;

    printf("\r\n=== STARTING MACRO: CLIMB +20 (ROBUST POLLING) ===\r\n");

    // --- PHASE 1: APPROACH STEP ---
    printf("PHASE 1: Approach Front...\r\n");
    VL53L1X_StopRanging(TOF_RACK_FRONT);
    VL53L1X_StartRanging(TOF_RACK_FRONT);

    while (dist_front > 300)
    {
        if (Check_Emergency_Stop()) return;

        Mechanum_DriveStraight(auto_speed);

        VL53L1X_GetRangeStatus(TOF_RACK_FRONT, &rangeStatus);

        // Only trust data if status is 0 (Success)
        if (rangeStatus == 0) {
            uint16_t raw_dist = 0;
            VL53L1X_GetDistance(TOF_RACK_FRONT, &raw_dist);
            // Ignore physically impossible long-range garbage
            if (raw_dist < 3000) {
                dist_front = raw_dist;
                error_count = 0; // Reset error counter on good read
            }
        } else {
            error_count++;
            if (error_count > 3) {
                printf("!!! SENSOR ERROR: Shutting down motors !!!\r\n");
                Mechanum_DriveStraight(0);
                return;
            }
        }

        printf("Front Dist: %d mm | Status: %d\r\n", dist_front, rangeStatus);
        HAL_Delay(25);
    }
    Mechanum_DriveStraight(0);
    HAL_Delay(500);

    // --- PHASE 2: LIFT FRONT ---
    printf("PHASE 2: Lift Front...\r\n");
    STS_WritePosition(huart_base, 120, STEPS_PLUS_20, 1500); HAL_Delay(300);
    STS_WritePosition(huart_base, 121, STEPS_PLUS_20, 1500); HAL_Delay(300);
    STS_WritePosition(huart_wrist, 124, STEPS_PLUS_20, 1500); HAL_Delay(300);
    STS_WritePosition(huart_wrist, 125, STEPS_PLUS_20, 1500); HAL_Delay(300);

    // --- PHASE 3: MOVE CHASSIS ---
    printf("PHASE 3: Move Rear...\r\n");
    VL53L1X_StopRanging(TOF_RACK_FRONT);
    VL53L1X_StartRanging(TOF_RACK_FRONT);
    error_count = 0;

    while (dist_rear > 300)
    {
        if (Check_Emergency_Stop()) return;
        Mechanum_DriveStraight(auto_speed);

        VL53L1X_GetRangeStatus(TOF_RACK_FRONT, &rangeStatus);
        if (rangeStatus == 0) {
            uint16_t raw_dist = 0;
            VL53L1X_GetDistance(TOF_RACK_FRONT, &raw_dist);
            if (raw_dist < 3000) {
                dist_rear = raw_dist;
                error_count = 0;
            }
        } else {
            error_count++;
            if (error_count > 10) { Mechanum_DriveStraight(0); return; }
        }
        printf("Rear Dist: %d mm | Status: %d\r\n", dist_rear, rangeStatus);
        HAL_Delay(25);
    }
    Mechanum_DriveStraight(0);
    HAL_Delay(500);

    // --- PHASE 4: LIFT REAR ---
    printf("PHASE 4: Lift Rear...\r\n");
    STS_WritePosition(huart_base, 122, STEPS_PLUS_20, 1500); HAL_Delay(300);
    STS_WritePosition(huart_base, 123, STEPS_PLUS_20, 1500); HAL_Delay(300);
    STS_WritePosition(huart_wrist, 126, STEPS_PLUS_20, 1500); HAL_Delay(300);
    STS_WritePosition(huart_wrist, 127, STEPS_PLUS_20, 1500); HAL_Delay(300);

    // --- PHASE 5: SECURE ---
    printf("PHASE 5: Secure...\r\n");
    dist_front = 0;
    while (dist_front < 300)
    {
        if (Check_Emergency_Stop()) return;
        Mechanum_DriveStraight(auto_speed);
        VL53L1X_GetDistance(TOF_RACK_FRONT, &dist_front);
        printf("Securing: %d mm\r\n", dist_front);
        HAL_Delay(25);
    }
    Mechanum_DriveStraight(0);
    printf("\r\n=== CLIMB COMPLETE ===\r\n");
}


void Macro_Climb_Plus20_EXTI(UART_HandleTypeDef *huart_base, UART_HandleTypeDef *huart_wrist)
{
    int32_t auto_speed = 5000;
    uint16_t avg_dist = 0;

    printf("\r\n=== STARTING MACRO: CLIMB +20 (EXTI + AVERAGE) ===\r\n");

    // --- PHASE 1: APPROACH STEP ---
    printf("PHASE 1: Approach Front...\r\n");

    // 1. Verify Sensor is alive
    uint16_t id = 0;
    VL53L1X_GetSensorId(TOF_RACK_FRONT, &id);
    if(id != 0xEACC) {
        printf("FATAL: Front sensor dead. Aborting.\r\n");
        return;
    }

    // 2. Set threshold to < 70mm and arm the EXTI Gate
    VL53L1X_SetDistanceThreshold(TOF_RACK_FRONT, 70, 0, 0, 0);
    VL53L1X_ClearInterrupt(TOF_RACK_FRONT);
    exti_triggered = 0;
    active_sensor_exti = TOF_RACK_FRONT;

    while (1)
    {
        if (Check_Emergency_Stop()) { active_sensor_exti = 0; return; }
        Mechanum_DriveStraight(auto_speed);

        // Hardware pulled the line LOW!
        if (exti_triggered == 1) {
            printf("Hardware INT received! Verifying...\r\n");
            Mechanum_DriveStraight(0); // Optional: Pre-emptive brake for safety

            // THE JUDGE: Take 5 samples to verify
            avg_dist = VL53L1X_GetStableAverage(TOF_RACK_FRONT, 5);

            if (avg_dist > 0 && avg_dist <= 70) {
                printf("Verified Stop! Final Dist: %d mm\r\n", avg_dist);
                Mechanum_DriveStraight(0); // Secure the stop
                break; // Exit the while loop
            } else {
                printf("False Positive (Dist: %d). Continuing drive...\r\n", avg_dist);
                // False alarm: reset flags and keep going
                exti_triggered = 0;
                VL53L1X_ClearInterrupt(TOF_RACK_FRONT);
            }
        }
    }
    active_sensor_exti = 0; // Close the gate
    HAL_Delay(500);

    // --- PHASE 2: LIFT FRONT ---
    printf("PHASE 2: Lift Front...\r\n");
    STS_WritePosition(huart_base, 120, STEPS_PLUS_20, 1500); HAL_Delay(300);
    STS_WritePosition(huart_base, 121, STEPS_PLUS_20, 1500); HAL_Delay(300);
    STS_WritePosition(huart_wrist, 124, STEPS_PLUS_20, 1500); HAL_Delay(300);
    STS_WritePosition(huart_wrist, 125, STEPS_PLUS_20, 1500); HAL_Delay(300);

    // --- PHASE 3: MOVE CHASSIS ---
    printf("PHASE 3: Move Rear...\r\n");

    VL53L1X_GetSensorId(TOF_RACK_FRONT, &id);
    if(id != 0xEACC) { printf("FATAL: Rear sensor dead.\r\n"); return; }

    // Set threshold for Rear (< 70mm) and Arm Gate
    VL53L1X_SetDistanceThreshold(TOF_RACK_FRONT, 70, 0, 0, 0);
    VL53L1X_ClearInterrupt(TOF_RACK_FRONT);
    exti_triggered = 0;
    active_sensor_exti = TOF_RACK_FRONT;

    while (1)
    {
        if (Check_Emergency_Stop()) { active_sensor_exti = 0; return; }
        Mechanum_DriveStraight(auto_speed);

        if (exti_triggered == 1) {
            avg_dist = VL53L1X_GetStableAverage(TOF_RACK_FRONT, 5);

            if (avg_dist > 0 && avg_dist <= 70) {
                printf("Verified Stop! Rear Dist: %d mm\r\n", avg_dist);
                Mechanum_DriveStraight(0);
                break;
            } else {
                exti_triggered = 0;
                VL53L1X_ClearInterrupt(TOF_RACK_FRONT);
            }
        }
    }
    active_sensor_exti = 0; // Close the gate
    HAL_Delay(500);

    // --- PHASE 4: LIFT REAR ---
    printf("PHASE 4: Lift Rear...\r\n");
    STS_WritePosition(huart_base, 122, STEPS_PLUS_20, 1500); HAL_Delay(300);
    STS_WritePosition(huart_base, 123, STEPS_PLUS_20, 1500); HAL_Delay(300);
    STS_WritePosition(huart_wrist, 126, STEPS_PLUS_20, 1500); HAL_Delay(300);
    STS_WritePosition(huart_wrist, 127, STEPS_PLUS_20, 1500); HAL_Delay(300);

    // --- PHASE 5: SECURE ---
    printf("PHASE 5: Secure...\r\n");

    // Dynamically change the threshold on the fly for the securement phase (< 300mm)
    VL53L1X_SetDistanceThreshold(TOF_RACK_FRONT, 300, 0, 0, 0);
    VL53L1X_ClearInterrupt(TOF_RACK_FRONT);
    exti_triggered = 0;
    active_sensor_exti = TOF_RACK_FRONT;

    while (1)
    {
        if (Check_Emergency_Stop()) { active_sensor_exti = 0; return; }
        Mechanum_DriveStraight(auto_speed);

        if (exti_triggered == 1) {
            avg_dist = VL53L1X_GetStableAverage(TOF_RACK_FRONT, 5);

            if (avg_dist > 0 && avg_dist <= 300) {
                printf("Securement Verified! Dist: %d mm\r\n", avg_dist);
                break;
            } else {
                exti_triggered = 0;
                VL53L1X_ClearInterrupt(TOF_RACK_FRONT);
            }
        }
    }
    Mechanum_DriveStraight(0);
    active_sensor_exti = 0;

    printf("\r\n=== CLIMB COMPLETE (EXTI) ===\r\n");
}

