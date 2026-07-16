#ifndef AccelStepper_h
#define AccelStepper_h

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

typedef bool boolean;

class AccelStepper {
public:
    typedef struct {
        GPIO_TypeDef* port;
        uint16_t pin;
    } STM32_Pin;

    typedef enum { DRIVER = 1 } MotorInterfaceType;

    AccelStepper(uint8_t interface = AccelStepper::DRIVER,
                 STM32_Pin pin1 = {0,0}, STM32_Pin pin2 = {0,0});

    void    moveTo(long absolute);
    boolean run();
    void    setMaxSpeed(float speed);
    void    setAcceleration(float acceleration);
    long    distanceToGo();
    long    currentPosition();
    void    computeNewSpeed();
    virtual void step(long step);
    virtual void enableOutputs();

private:
    uint8_t   _interface;
    STM32_Pin _pin[2];
    long      _currentPos, _targetPos, _n;
    float     _speed, _maxSpeed, _acceleration, _c0, _cn, _cmin;
    uint32_t  _stepInterval, _lastStepTime;
    bool      _direction;
    uint32_t  getMicros();
};
#endif
