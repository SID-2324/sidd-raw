#include <Arduino.h>
#include "SCServo.h" 
#include "SMS_STS.h" 

// --- ONBOARD RGB (ESP32-S3) ---
#define RGB_PIN 48         // Standard for S3 N16R8
#define RGB_BRIGHTNESS 30  // 0-255 (Keep it low, these are bright!)

// --- DRIVER 1 (UART2) ---
#define D1_RX 16
#define D1_TX 17
const byte D1_SERVO_ID_1 = 0;
const byte D1_SERVO_ID_2 = 1;

// --- DRIVER 2 (UART1) ---
// Note: On S3, we can use 18/19 or 4/5 safely
#define D2_RX 18
#define D2_TX 19
const byte D2_SERVO_ID_1 = 0;
const byte D2_SERVO_ID_2 = 1;

SMS_STS driver1;
SMS_STS driver2;

HardwareSerial SerialBus1(2);
HardwareSerial SerialBus2(1);

// ===== MECHANISM CONSTANTS =====
const int STEPS_PER_REV = 4096;
const float CM_PER_REV  = 2.0; 
const int SPEED = 500;
const int ACCEL = 50; 

long posD1_S1 = 0, posD1_S2 = 0;
long posD2_S1 = 0, posD2_S2 = 0;

long distanceToSteps(float cm) {
  return (long)(cm * (4096.0 / CM_PER_REV));
}

void setup() {
  Serial.begin(115200);
  
  // Initialize RGB LED (Uses built-in S3 neopixelWrite)
  // Turn Blue to show "Power On"
  neopixelWrite(RGB_PIN, 0, 0, RGB_BRIGHTNESS); 
  
  delay(2000);

  // Initialize Bus 1
  SerialBus1.begin(1000000, SERIAL_8N1, D1_RX, D1_TX);
  driver1.pSerial = &SerialBus1;
  
  // Initialize Bus 2
  SerialBus2.begin(1000000, SERIAL_8N1, D2_RX, D2_TX);
  driver2.pSerial = &SerialBus2;

  Serial.println("✅ ESP32-S3 System Ready.");
  Serial.println("Enter distance (cm):");
  
  // Fade RGB to a dim Purple to show "Ready"
  neopixelWrite(RGB_PIN, RGB_BRIGHTNESS/2, 0, RGB_BRIGHTNESS);
}

void loop() {
  if (Serial.available()) {
    float dist = Serial.parseFloat();
    if (dist == 0.0) return;

    // Flash Green to acknowledge input
    neopixelWrite(RGB_PIN, 0, RGB_BRIGHTNESS, 0);

    long steps = distanceToSteps(dist);

    posD1_S1 += steps; posD1_S2 += steps;
    posD2_S1 += steps; posD2_S2 += steps;

    driver1.WritePosEx(D1_SERVO_ID_1, posD1_S1, SPEED, ACCEL);
    driver1.WritePosEx(D1_SERVO_ID_2, posD1_S2, SPEED, ACCEL);
    driver2.WritePosEx(D2_SERVO_ID_1, posD2_S1, SPEED, ACCEL);
    driver2.WritePosEx(D2_SERVO_ID_2, posD2_S2, SPEED, ACCEL);

    Serial.printf("Moving 4 servos %.2f cm...\n", dist);

    delay(200); 

    while (driver1.ReadMove(D1_SERVO_ID_1) > 0 || driver2.ReadMove(D2_SERVO_ID_1) > 0) {
      delay(50);
    }
    
    // Return to "Ready" Purple
    neopixelWrite(RGB_PIN, RGB_BRIGHTNESS/2, 0, RGB_BRIGHTNESS);
    Serial.println("✅ Target Reached.");
  }
}