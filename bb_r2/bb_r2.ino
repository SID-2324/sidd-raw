#include <AccelStepper.h>
#include <Wire.h>
#include <PS4Controller.h>

#define I2C_SDA 21
#define I2C_SCL 22
#define SLAVE_ADDR 0x05

#define STEP_PIN 27
#define DIR_PIN 14
#define STEPS_PER_REV (200 * 19)  // Motor steps * gear ratio
#define MICROSTEPS 1 

AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
bool pos = false;

int angleToSteps(int angle) {
  return (angle * MICROSTEPS * STEPS_PER_REV) / 360;
}

TaskHandle_t steppertask;

struct PS4Data {
  bool Square;
  bool Cross;
  bool Circle;
  bool Triangle;
  bool touchpad;
  bool Up;
  bool Down;
  bool Left;
  bool Right;
  bool L1;
  bool R1;
  int LStickX;
  int LStickY;
  int RStickX;
  int RStickY;
  int L2;
  int R2;
};

PS4Data ps4Data;

void steppermot(void *parameters) {
  static bool lastCross = false;
  while (1) {
    if (PS4.Cross()) {
      if (!lastCross) {
        pos = !pos;
        int targetSteps = pos ? angleToSteps(-180) : angleToSteps(180);  // ✅ Changed from 50 to 45 degrees
        stepper.moveTo(targetSteps);
        while (stepper.distanceToGo()) {
          stepper.run();
        }
        stepper.setCurrentPosition(0);  // Reset position to 0 after each move
      }
      lastCross = true;
    } else {
      lastCross = false;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL, 200000);
  PS4.begin("18:b5:cd:16:d6:5a");  // Change MAC address if needed

  // Stepper configuration
  stepper.setMaxSpeed(8000);
  stepper.setAcceleration(6000);
  stepper.setSpeed(8000);
  stepper.setCurrentPosition(0);

  xTaskCreate(steppermot, "stepper", 4096, NULL, 1, &steppertask);
}

void loop() {
  if (PS4.isConnected()) {
    ps4Data.LStickX = PS4.LStickX();
    ps4Data.LStickY = PS4.LStickY();
    ps4Data.RStickX = PS4.RStickX();
    ps4Data.RStickY = PS4.RStickY();
    ps4Data.Square = PS4.Square();
    ps4Data.Cross = PS4.Cross();
    ps4Data.Circle = PS4.Circle();
    ps4Data.Triangle = PS4.Triangle();
    ps4Data.touchpad = PS4.Touchpad();
    ps4Data.Up = PS4.Up();
    ps4Data.Down = PS4.Down();
    ps4Data.Left = PS4.Left();
    ps4Data.Right = PS4.Right();
    ps4Data.L1 = PS4.L1();
    ps4Data.R1 = PS4.R1();
    ps4Data.L2 = PS4.L2Value();
    ps4Data.R2 = PS4.R2Value();

    Wire.beginTransmission(SLAVE_ADDR);
    Wire.write((uint8_t*)&ps4Data, sizeof(ps4Data)); 
    Wire.endTransmission();
  }
}
