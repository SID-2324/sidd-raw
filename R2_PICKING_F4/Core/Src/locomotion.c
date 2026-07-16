#include "locomotion.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
// Global variables for encoder ticks
int32_t my_FL = 0;
int32_t my_FR = 0;
int32_t my_RL = 0;
int32_t my_RR = 0;

// Helper: Convert raw ticks to RPM
// Assuming 50ms (0.05s) sample time
float Calculate_RPM(int16_t delta_ticks) {
    const float PPR = 538.13f;
    const float sample_time_seconds = 0.05f;
    return (fabs((float)delta_ticks) / PPR) * (1.0f / sample_time_seconds) * 60.0f;
}

void Drive_Motor_Hardware(uint32_t channel, GPIO_TypeDef *dir_port, uint16_t dir_pin, int32_t speed)
{
    if (channel == MOTOR_FL_PWM_CH)      speed *= MOTOR_FL_INVERT;
    else if (channel == MOTOR_FR_PWM_CH) speed *= MOTOR_FR_INVERT;
    else if (channel == MOTOR_RL_PWM_CH) speed *= MOTOR_RL_INVERT;
    else if (channel == MOTOR_RR_PWM_CH) speed *= MOTOR_RR_INVERT;

    uint32_t absolute_speed = (speed < 0) ? -speed : speed;

    if (speed >= 0) {
        HAL_GPIO_WritePin(dir_port, dir_pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(dir_port, dir_pin, GPIO_PIN_RESET);
    }
    __HAL_TIM_SET_COMPARE(&htim2, channel, absolute_speed);
}

void STS_MecanumDualJoystickDrive(int16_t left_y, int16_t left_x, int16_t right_x)
{
    if (left_y > -JOY_DEADZONE  && left_y < JOY_DEADZONE)  left_y = 0;
    if (left_x > -JOY_DEADZONE  && left_x < JOY_DEADZONE)  left_x = 0;
    if (right_x > -JOY_DEADZONE && right_x < JOY_DEADZONE) right_x = 0;

    int16_t snap_threshold = 150;
    if (abs(left_y) > snap_threshold && abs(left_x) < snap_threshold) left_x = 0;
    else if (abs(left_x) > snap_threshold && abs(left_y) < snap_threshold) left_y = 0;

    int16_t fl_raw = left_y - right_x - left_x;
    int16_t fr_raw = left_y + right_x + left_x;
    int16_t rl_raw = -left_y + right_x - left_x;
    int16_t rr_raw = left_y + right_x - left_x;

    int16_t max_val = abs(fl_raw);
    if (abs(fr_raw) > max_val) max_val = abs(fr_raw);
    if (abs(rl_raw) > max_val) max_val = abs(rl_raw);
    if (abs(rr_raw) > max_val) max_val = abs(rr_raw);

    if (max_val > 512) {
        fl_raw = (fl_raw * 512) / max_val;
        fr_raw = (fr_raw * 512) / max_val;
        rl_raw = (rl_raw * 512) / max_val;
        rr_raw = (rr_raw * 512) / max_val;
    }

    Drive_Motor_Hardware(MOTOR_FL_PWM_CH, MOTOR_FL_DIR_PORT, MOTOR_FL_DIR_PIN, ((int32_t)fl_raw * MOTOR_SPEED_MAX) / 512);
    Drive_Motor_Hardware(MOTOR_FR_PWM_CH, MOTOR_FR_DIR_PORT, MOTOR_FR_DIR_PIN, ((int32_t)fr_raw * MOTOR_SPEED_MAX) / 512);
    Drive_Motor_Hardware(MOTOR_RL_PWM_CH, MOTOR_RL_DIR_PORT, MOTOR_RL_DIR_PIN, ((int32_t)rl_raw * MOTOR_SPEED_MAX) / 512);
    Drive_Motor_Hardware(MOTOR_RR_PWM_CH, MOTOR_RR_DIR_PORT, MOTOR_RR_DIR_PIN, ((int32_t)rr_raw * MOTOR_SPEED_MAX) / 512);
}

void Print_Raw_Encoder_Ticks(int32_t ticks_FL, int32_t ticks_FR, int32_t ticks_RL, int32_t ticks_RR) {
    printf("Raw Ticks -> FL: %ld | FR: %ld | RL: %ld | RR: %ld\r\n", ticks_FL, ticks_FR, ticks_RL, ticks_RR);
}

void Read_All_Encoders(int32_t *fl, int32_t *fr, int32_t *rl, int32_t *rr)
{
    *fl = (int16_t)__HAL_TIM_GET_COUNTER(&htim1);
    *fr = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
    *rl = (int16_t)__HAL_TIM_GET_COUNTER(&htim4);
    *rr = (int16_t)__HAL_TIM_GET_COUNTER(&htim8);
}

void Mechanum_DriveStraight(int32_t speed)
{
    Drive_Motor_Hardware(MOTOR_FL_PWM_CH, MOTOR_FL_DIR_PORT, MOTOR_FL_DIR_PIN, speed);//-
    Drive_Motor_Hardware(MOTOR_FR_PWM_CH, MOTOR_FR_DIR_PORT, MOTOR_FR_DIR_PIN, speed);//-
    Drive_Motor_Hardware(MOTOR_RL_PWM_CH, MOTOR_RL_DIR_PORT, MOTOR_RL_DIR_PIN, -speed);//+
    Drive_Motor_Hardware(MOTOR_RR_PWM_CH, MOTOR_RR_DIR_PORT, MOTOR_RR_DIR_PIN, speed);//-
}

void Mechanum_DriveBackward(int32_t speed)
{
    Drive_Motor_Hardware(MOTOR_FL_PWM_CH, MOTOR_FL_DIR_PORT, MOTOR_FL_DIR_PIN, -speed);
    Drive_Motor_Hardware(MOTOR_FR_PWM_CH, MOTOR_FR_DIR_PORT, MOTOR_FR_DIR_PIN, -speed);
    Drive_Motor_Hardware(MOTOR_RL_PWM_CH, MOTOR_RL_DIR_PORT, MOTOR_RL_DIR_PIN, speed);
    Drive_Motor_Hardware(MOTOR_RR_PWM_CH, MOTOR_RR_DIR_PORT, MOTOR_RR_DIR_PIN, -speed);
}

void Mechanum_Drive_Right_Spot_rotation(int32_t speed)
{
    Drive_Motor_Hardware(MOTOR_FL_PWM_CH, MOTOR_FL_DIR_PORT, MOTOR_FL_DIR_PIN, -speed);
    Drive_Motor_Hardware(MOTOR_FR_PWM_CH, MOTOR_FR_DIR_PORT, MOTOR_FR_DIR_PIN, speed);
    Drive_Motor_Hardware(MOTOR_RL_PWM_CH, MOTOR_RL_DIR_PORT, MOTOR_RL_DIR_PIN, speed);
    Drive_Motor_Hardware(MOTOR_RR_PWM_CH, MOTOR_RR_DIR_PORT, MOTOR_RR_DIR_PIN, speed);
}

void Mechanum_Drive_Left_Spot_rotation(int32_t speed)
{
    Drive_Motor_Hardware(MOTOR_FL_PWM_CH, MOTOR_FL_DIR_PORT, MOTOR_FL_DIR_PIN, speed);
    Drive_Motor_Hardware(MOTOR_FR_PWM_CH, MOTOR_FR_DIR_PORT, MOTOR_FR_DIR_PIN, -speed);
    Drive_Motor_Hardware(MOTOR_RL_PWM_CH, MOTOR_RL_DIR_PORT, MOTOR_RL_DIR_PIN, -speed);
    Drive_Motor_Hardware(MOTOR_RR_PWM_CH, MOTOR_RR_DIR_PORT, MOTOR_RR_DIR_PIN, -speed);
}
void Mechanum_Drive_Left_Pan(int32_t speed)
{
    Drive_Motor_Hardware(MOTOR_FL_PWM_CH, MOTOR_FL_DIR_PORT, MOTOR_FL_DIR_PIN, -speed);
    Drive_Motor_Hardware(MOTOR_FR_PWM_CH, MOTOR_FR_DIR_PORT, MOTOR_FR_DIR_PIN, speed);
    Drive_Motor_Hardware(MOTOR_RL_PWM_CH, MOTOR_RL_DIR_PORT, MOTOR_RL_DIR_PIN, -speed);
    Drive_Motor_Hardware(MOTOR_RR_PWM_CH, MOTOR_RR_DIR_PORT, MOTOR_RR_DIR_PIN, -speed);
}
void Mechanum_Drive_Right_Pan(int32_t speed)
{
    Drive_Motor_Hardware(MOTOR_FL_PWM_CH, MOTOR_FL_DIR_PORT, MOTOR_FL_DIR_PIN, speed);
    Drive_Motor_Hardware(MOTOR_FR_PWM_CH, MOTOR_FR_DIR_PORT, MOTOR_FR_DIR_PIN, -speed);
    Drive_Motor_Hardware(MOTOR_RL_PWM_CH, MOTOR_RL_DIR_PORT, MOTOR_RL_DIR_PIN, speed);
    Drive_Motor_Hardware(MOTOR_RR_PWM_CH, MOTOR_RR_DIR_PORT, MOTOR_RR_DIR_PIN, speed);
}

void Mechanum_DriveStraight_Matched(int32_t base_pwm)
{
    static uint16_t prev_FL = 0, prev_FR = 0, prev_RL = 0, prev_RR = 0;

    uint16_t curr_FL = (uint16_t)__HAL_TIM_GET_COUNTER(&htim1);
    uint16_t curr_FR = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);
    uint16_t curr_RL = (uint16_t)__HAL_TIM_GET_COUNTER(&htim4);
    uint16_t curr_RR = (uint16_t)__HAL_TIM_GET_COUNTER(&htim8);

    int16_t speed_FL = (int16_t)(curr_FL - prev_FL);
    int16_t speed_FR = (int16_t)(curr_FR - prev_FR);
    int16_t speed_RL = (int16_t)(curr_RL - prev_RL);
    int16_t speed_RR = (int16_t)(curr_RR - prev_RR);

    prev_FL = curr_FL; prev_FR = curr_FR; prev_RL = curr_RL; prev_RR = curr_RR;

    float rpm_FL = Calculate_RPM(speed_FL);
    float rpm_FR = Calculate_RPM(speed_FR);
    float rpm_RL = Calculate_RPM(speed_RL);
    float rpm_RR = Calculate_RPM(speed_RR);

    float avg_rpm = (rpm_FL + rpm_FR + rpm_RL + rpm_RR) / 4.0f;
    float Kp = 5.0f; // Adjusted for RPM tuning

    int32_t out_FL = base_pwm + (int32_t)((avg_rpm - rpm_FL) * Kp);
    int32_t out_FR = base_pwm + (int32_t)((avg_rpm - rpm_FR) * Kp);
    int32_t out_RL = base_pwm + (int32_t)((avg_rpm - rpm_RL) * Kp);
    int32_t out_RR = base_pwm + (int32_t)((avg_rpm - rpm_RR) * Kp);

    int32_t max_pwm = MOTOR_SPEED_MAX;
    if(out_FL > max_pwm) out_FL = max_pwm; else if(out_FL < 0) out_FL = 0;
    if(out_FR > max_pwm) out_FR = max_pwm; else if(out_FR < 0) out_FR = 0;
    if(out_RL > max_pwm) out_RL = max_pwm; else if(out_RL < 0) out_RL = 0;
    if(out_RR > max_pwm) out_RR = max_pwm; else if(out_RR < 0) out_RR = 0;

    Drive_Motor_Hardware(MOTOR_FL_PWM_CH, MOTOR_FL_DIR_PORT, MOTOR_FL_DIR_PIN, -out_FL);
    Drive_Motor_Hardware(MOTOR_FR_PWM_CH, MOTOR_FR_DIR_PORT, MOTOR_FR_DIR_PIN, -out_FR);
    Drive_Motor_Hardware(MOTOR_RL_PWM_CH, MOTOR_RL_DIR_PORT, MOTOR_RL_DIR_PIN,  out_RL);
    Drive_Motor_Hardware(MOTOR_RR_PWM_CH, MOTOR_RR_DIR_PORT, MOTOR_RR_DIR_PIN, -out_RR);


}

