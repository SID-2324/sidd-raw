#include "STSServoDriver.h"

STSServoDriver servos;

#define SERVO_ID 2

// ESP32 UART2 pins (adjust if needed)
#define SERVO_RX 16
#define SERVO_TX 17

// Direction pin (set to 255 if NOT needed)
#define DIR_PIN 255  

HardwareSerial ServoSerial(2);

void setup()
{
  Serial.begin(115200);
  delay(3000);

  // Initialize servo UART
  ServoSerial.begin(1000000, SERIAL_8N1, SERVO_RX, SERVO_TX);

  // Init driver (NO direction pin)
  if (!servos.init(&ServoSerial))
  {
    Serial.println("❌ Servo init failed");
    while (1);
  }

  Serial.println("✅ Servo connected");

  // Set velocity mode
  servos.setMode(SERVO_ID, STSMode::VELOCITY);
}

void loop()
{
  // Clockwise
  Serial.println("CW rotation");
  servos.setTargetVelocity(SERVO_ID, 500);  // steps/sec
  delay(2000);

  // Counter-clockwise
  Serial.println("CCW rotation");
  servos.setTargetVelocity(SERVO_ID, -500);
  delay(2000);
}