#include <PS4Controller.h>
#include<stdio.h>
// Connect MDDS10 'IN1' pin to ESP32's 'TX2' pin (GPIO 17)
#define RXD2 16 // Unused, but part of the Serial2 definition
#define TXD2 17

int deadzone = 10;

void setup() {
  // Start Serial2 for the motor driver
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // Replace with your PS4 Controller's MAC Address
  PS4.begin("d0:40:57:b6:f4:d0"); 
}

void loop() {
  if (PS4.isConnected()) {
    
    // 1. Read Joysticks (PS4 Y-axis is inverted)
    int throttle = -PS4.LStickY(); 
    int steering = PS4.RStickX();

    // 2. Apply Deadzone
    //TESTING
    scanf("%d", &throttle);
    scanf("%d", &steering);
    if (abs(throttle) < deadzone) throttle = 0;
    if (abs(steering) < deadzone) steering = 0;

    // 3. Mix for Tank Control
    int leftSpeed = throttle + steering;
    int rightSpeed = throttle - steering;

    // 4. Constrain (This is essential!)
    // Clamps the speed to the max joystick range before mapping
    leftSpeed = constrain(leftSpeed, -127, 127);
    rightSpeed = constrain(rightSpeed, -127, 127);

    // 5. Map to UART ranges and Send
    // Motor 1 (Left): 1 (Rev) - 64 (Stop) - 127 (Fwd)
    Serial2.write(map(leftSpeed, -127, 127, 127, 1));

    // Motor 2 (Right): 129 (Rev) - 192 (Stop) - 255 (Fwd)
    Serial2.write(map(rightSpeed, -127, 127, 255, 129));
    
  } else {
    // If controller disconnects, send stop commands
    Serial2.write(64);  // Stop Motor 1
    Serial2.write(192); // Stop Motor 2
    delay(20); // Small delay to prevent flooding
  }
}