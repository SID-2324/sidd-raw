/*
 * pid_v1.h
 *
 *  Created on: Apr 17, 2026
 *      Author: LENOVO
 */

#ifndef INC_PID_V1_H_
#define INC_PID_V1_H_

#include <stdint.h>

class PID
{
  public:
    // Constants
    static const int AUTOMATIC = 1;
    static const int MANUAL    = 0;
    static const int DIRECT    = 0;
    static const int REVERSE   = 1;
    static const int P_ON_M    = 0;
    static const int P_ON_E    = 1;

    // Constructors
    PID(double* Input, double* Output, double* Setpoint,
        double Kp, double Ki, double Kd, int POn, int ControllerDirection);

    PID(double* Input, double* Output, double* Setpoint,
        double Kp, double Ki, double Kd, int ControllerDirection);

    void SetMode(int Mode);
    bool Compute(uint32_t currentMillis); // Pass HAL_GetTick() here
    void SetOutputLimits(double Min, double Max);
    void SetTunings(double Kp, double Ki, double Kd);
    void SetTunings(double Kp, double Ki, double Kd, int POn);
    void SetControllerDirection(int Direction);
    void SetSampleTime(int NewSampleTime);
    void Initialize(uint32_t currentMillis);

    // Display functions
    double GetKp();
    double GetKi();
    double GetKd();
    double GetTi();
    double GetTd();
    int GetMode();
    int GetDirection();

    double outputSum;

  private:
    double dispKp, dispKi, dispKd;
    double kp, ki, kd;
    int controllerDirection;
    int pOn;

    double *myInput, *myOutput, *mySetpoint;
    uint32_t lastTime;
    double lastInput;
    uint32_t SampleTime;
    double outMin, outMax;
    bool inAuto, pOnE;
};



#endif /* INC_PID_V1_H_ */
