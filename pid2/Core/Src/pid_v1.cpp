#include "pid_v1.h"

PID::PID(double* Input, double* Output, double* Setpoint,
        double Kp, double Ki, double Kd, int POn, int ControllerDirection)
{
    myOutput = Output;
    myInput = Input;
    mySetpoint = Setpoint;
    inAuto = false;

    PID::SetOutputLimits(0, 255);
    SampleTime = 100; // default 100ms
    PID::SetControllerDirection(ControllerDirection);
    PID::SetTunings(Kp, Ki, Kd, POn);
    lastTime = 0;
}

PID::PID(double* Input, double* Output, double* Setpoint,
        double Kp, double Ki, double Kd, int ControllerDirection)
    : PID(Input, Output, Setpoint, Kp, Ki, Kd, P_ON_E, ControllerDirection)
{
}

bool PID::Compute(uint32_t currentMillis)
{
   if(!inAuto) return false;

   uint32_t timeChange = (currentMillis - lastTime);
   if(timeChange >= SampleTime)
   {
      double input = *myInput;
      double error = *mySetpoint - input;
      double dInput = (input - lastInput);
      outputSum += (ki * error);

      if(!pOnE) outputSum -= kp * dInput;

      if(outputSum > outMax) outputSum = outMax;
      else if(outputSum < outMin) outputSum = outMin;

      double output;
      if(pOnE) output = kp * error;
      else output = 0;

      output += outputSum - kd * dInput;

      if(output > outMax) {
          outputSum -= output - outMax;
          output = outMax;
      }
      else if(output < outMin) {
          outputSum += outMin - output;
          output = outMin;
      }
      *myOutput = output;

      lastInput = input;
      lastTime = currentMillis;
      return true;
   }
   return false;
}

void PID::SetTunings(double Kp, double Ki, double Kd, int POn)
{
   if (Kp < 0 || Ki < 0 || Kd < 0) return;

   pOn = POn;
   pOnE = (POn == P_ON_E);
   dispKp = Kp; dispKi = Ki; dispKd = Kd;

   double SampleTimeInSec = ((double)SampleTime) / 1000.0;
   kp = Kp;
   ki = Ki * SampleTimeInSec;
   kd = Kd / SampleTimeInSec;

   if(controllerDirection == REVERSE) {
      kp = (0 - kp);
      ki = (0 - ki);
      kd = (0 - kd);
   }
}

void PID::SetTunings(double Kp, double Ki, double Kd) {
    SetTunings(Kp, Ki, Kd, pOn);
}

void PID::SetSampleTime(int NewSampleTime)
{
   if (NewSampleTime > 0) {
      double ratio = (double)NewSampleTime / (double)SampleTime;
      ki *= ratio;
      kd /= ratio;
      SampleTime = (uint32_t)NewSampleTime;
   }
}

void PID::SetOutputLimits(double Min, double Max)
{
   if(Min >= Max) return;
   outMin = Min;
   outMax = Max;

   if(inAuto) {
       if(*myOutput > outMax) *myOutput = outMax;
       else if(*myOutput < outMin) *myOutput = outMin;

       if(outputSum > outMax) outputSum = outMax;
       else if(outputSum < outMin) outputSum = outMin;
   }
}

void PID::SetMode(int Mode)
{
    bool newAuto = (Mode == AUTOMATIC);
    // Note: To use Initialize() here, you'd need to track current time.
    // It's safer to call Initialize manually or pass time to SetMode.
    inAuto = newAuto;
}

void PID::Initialize(uint32_t currentMillis)
{
   outputSum = *myOutput;
   lastInput = *myInput;
   lastTime = currentMillis;
   if(outputSum > outMax) outputSum = outMax;
   else if(outputSum < outMin) outputSum = outMin;
}

void PID::SetControllerDirection(int Direction)
{
   if(inAuto && Direction != controllerDirection) {
      kp = (0 - kp);
      ki = (0 - ki);
      kd = (0 - kd);
   }
   controllerDirection = Direction;
}

double PID::GetKp(){ return dispKp; }
double PID::GetKi(){ return dispKi; }
double PID::GetTi(){ return dispKp/dispKi; }
double PID::GetKd(){ return dispKd; }
double PID::GetTd(){ return dispKd/dispKp; }
int PID::GetMode(){ return inAuto ? AUTOMATIC : MANUAL; }
int PID::GetDirection(){ return controllerDirection; }
