#include <PS4Controller.h>

// UART TX PINS
#define UART_L_TX 16 
#define UART_R_TX 17 
#define SPEED     5  // 0–31

void sendCommand(uint8_t id, bool reverse, uint8_t spd) {
  // Build packet: [ID(2 bits) | DIR(1 bit) | SPEED(5 bits)]
  uint8_t data = (id << 6) | (reverse << 5) | (spd & 0x1F);
  
  // IDs 0-1 go to Serial1 (Left), IDs 2-3 go to Serial2 (Right)
  if (id < 2) Serial1.write(data); 
  else        Serial2.write(data);
}

void drive(uint8_t spd) {
  // spd > 0 moves forward; spd == 0 stops.
  // DIR_CCW (1) for left side, DIR_CW (0) for right side for forward motion
  sendCommand(0, 1, spd); // Front Left
  sendCommand(1, 1, spd); // Rear Left
  sendCommand(2, 0, spd); // Front Right
  sendCommand(3, 0, spd); // Rear Right
}

void setup() {
  Serial.begin(115200);
  PS4.begin("d0:39:57:b6:f4:d1");
  
  // Initialize UARTs (TX only)
  Serial1.begin(115200, SERIAL_8N1, -1, UART_L_TX);
  Serial2.begin(115200, SERIAL_8N1, -1, UART_R_TX);
}

void loop() {
  if (PS4.isConnected()) {
    if (PS4.Circle()) {
    drive(SPEED);
    }
    
    else if (PS4.Triangle()) {
    drive(0);
  }
  }

  
  delay(20);
}