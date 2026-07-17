#include <HardwareSerial.h>

#include <Wire.h>

#define SLAVE_ADDR 0x55

void setup() {
  Serial.begin(115200);
  Wire.begin(SLAVE_ADDR);
  Wire.onRequest(requestEvent);
  Serial.println("Slave Ready at 0x55");
  
  MDDS_Serial.begin(9600 , SERIAL_8N1 , 16 , 17);
  delay(2000);
  MDDS_Serial.write(85);
  delay(1000);
}

void loop() {
  delay(1); // Keep loop empty
}

void requestEvent() {
  // We use a char array to ensure the buffer is formatted correctly
  char message[] = "Hi Mstr"; 
  Wire.write((uint8_t *)message, 7); 
  
  Serial.println("Request handled: 'Hi Mstr' sent to Master");
}