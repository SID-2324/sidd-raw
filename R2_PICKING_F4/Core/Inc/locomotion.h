// ==========================================
// locomotion.h
// ==========================================
#ifndef INC_LOCOMOTION_H_
#define INC_LOCOMOTION_H_

#include "main.h"

// --- HARDWARE INVERSION FLAGS ---
// Change to -1 for the specific motors that are spinning backward on the PCB
#define MOTOR_FL_INVERT    1
#define MOTOR_FR_INVERT    -1
#define MOTOR_RL_INVERT    1
#define MOTOR_RR_INVERT    1

// Hardware Pin Configuration
#define MOTOR_FL_DIR_PORT  GPIOE
#define MOTOR_FL_DIR_PIN   GPIO_PIN_6          // PC0: DIGI1 //fr
#define MOTOR_FL_PWM_CH    TIM_CHANNEL_2       // PA2: ANA1

// MDDS10 2 - Front Right Motor
#define MOTOR_FR_DIR_PORT  GPIOE
#define MOTOR_FR_DIR_PIN   GPIO_PIN_5          // PC2: DIGI2 //fl
#define MOTOR_FR_PWM_CH    TIM_CHANNEL_1       // PA3: ANA2

// MDDS10 1 - Rear Left Motor
#define MOTOR_RL_DIR_PORT  GPIOC
#define MOTOR_RL_DIR_PIN   GPIO_PIN_0          // PE4: DIGI1
#define MOTOR_RL_PWM_CH    TIM_CHANNEL_3       // PA0: ANA1

// MDDS10 1 - Rear Right Motor
#define MOTOR_RR_DIR_PORT  GPIOC
#define MOTOR_RR_DIR_PIN   GPIO_PIN_2          // PE6: DIGI2
#define MOTOR_RR_PWM_CH    TIM_CHANNEL_4

#define JOY_DEADZONE       10
#define MOTOR_SPEED_MAX    10000


extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim8;
// Add these near your other externs (like the htims)
extern int32_t my_FL;
extern int32_t my_FR;
extern int32_t my_RL;
extern int32_t my_RR;

void Drive_Motor_Hardware(uint32_t channel, GPIO_TypeDef *dir_port, uint16_t dir_pin, int32_t speed);
void STS_MecanumDualJoystickDrive(int16_t left_y, int16_t left_x, int16_t right_x);
void Print_Raw_Encoder_Ticks(int32_t ticks_FL, int32_t ticks_FR, int32_t ticks_RL, int32_t ticks_RR);
void Mechanum_DriveStraight_Matched(int32_t base_speed);
void Read_All_Encoders(int32_t *fl, int32_t *fr, int32_t *rl, int32_t *rr);
void Mechanum_DriveStraight(int32_t speed);
void Mechanum_DriveBackward(int32_t speed);
void Mechanum_Drive_Left_Spot_rotation(int32_t speed);
void Mechanum_Drive_Right_Spot_rotation(int32_t speed);
#endif /* INC_LOCOMOTION_H_ */
