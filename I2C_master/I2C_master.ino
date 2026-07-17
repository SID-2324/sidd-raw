#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin(); // Standard SDA (21) and SCL (22)
  Serial.println("I2C Master Initialized");
}

void loop() {
  Serial.println("Requesting data...");
  
  // Request 7 bytes from Slave at address 0x55
  uint8_t bytesReceived = Wire.requestFrom(0x55, 7);

  if (bytesReceived > 0) {
    Serial.print("Received: ");
    while (Wire.available()) {
      char c = Wire.read();
      Serial.print(c);
    }
    Serial.println(); 
  } else {
    // This runs if Slave triggers but Master gets 0 bytes
    Serial.println("Failed! No data received from Slave.");
  }

  delay(2000); 
}