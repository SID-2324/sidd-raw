#ifndef INC_PICKING_AUTONOMOUS_H_
#define INC_PICKING_AUTONOMOUS_H_

#include "main.h"
#include "vl53l1_platform.h"
#include "VL53L1X_api.h"
#include "locomotion.h" // Required for driving the wheels

#define NUM_SENSORS 2

// ⚠️ Map your addresses to their physical jobs on the bot
//#define TOF_SPEARHEAD_FRONT  0x54
//#define TOF_GRIPPER_INSIDE   0x56
#define TOF_CHASSIS_LEFT     0x58
#define TOF_CHASSIS_RIGHT    0x5A
#define TOF_DROP_ZONE_ALIGN  0x5C
// (Add the remaining 4 sensors)
// --- TOF SENSOR MAPPING ---
#define TOF_RACK_FRONT 0x56  // Update to actual Front Rack ToF Address
#define TOF_EFFECTOR_FRONT  0x54  // Update to actual Rear Rack ToF Address


// --- CLIMBING CALIBRATIONS ---
#define STEPS_PLUS_20   1000
#define STEPS_PLUS_40   2000
#define CLIMB_SPEED     4000
// --- JETSON MACRO FUNCTIONS ---
void Macro_PickSpearhead(UART_HandleTypeDef *huart_base, UART_HandleTypeDef *huart_wrist);
void Macro_AlignDropZone(UART_HandleTypeDef *huart_jetson);
void Macro_StrafeToNextRack(UART_HandleTypeDef *huart_jetson);
void Macro_Climb_Plus20(UART_HandleTypeDef *huart_base, UART_HandleTypeDef *huart_wrist);
void Macro_Climb_Plus20_EXTI(UART_HandleTypeDef *huart_base, UART_HandleTypeDef *huart_wrist);
uint8_t Check_Emergency_Stop(void);
// --- MASTER ROUTER ---
//void Execute_Jetson_Command(char cmd, UART_HandleTypeDef *huart_jetson);

#endif /* INC_PICKING_AUTONOMOUS_H_ */
