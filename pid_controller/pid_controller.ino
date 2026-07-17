#include <HardwareSerial.h>
HardwareSerial MDDS_Serial(2);


void setup() {

  MDDS_Serial.begin(9600, SERIAL_8N1,16,17);

  
  delay(2000);

 
  MDDS_Serial.write(85);
  
  
  delay(1000);
}

void loop() {
  
 
}

void PIDcontroller(int target , long current , float prevError , float Integral , float dt){
  float error = target - current;
  float derivative = (error - prevError)/dt;
  integral= integral + (error *dt);
  float output =
}


void controlMotor(int address, int channel, int speed) {
  
  // Calculate Byte 2: (Channel * 8) + Address
  byte byte2 = (channel * 8) + address;
  
  // Calculate Checksum: Header + Byte2 + Speed
  byte checksum = 85 + byte2 + speed;

  // Send the Packet
  MDDS_Serial.write(85);     
  MDDS_Serial.write(byte2);    // 
  MDDS_Serial.write(speed);    // 
  MDDS_Serial.write(checksum); 
}


void stopmotors(){
  controlMotor(0, 0, 127); // Stop D1 Left
  controlMotor(0, 1, 127); // Stop D1 Right
  controlMotor(1, 0, 127); // Stop D2 Left
  controlMotor(1, 1, 127); // Stop D2 Right

}

void Forward(){
  //forward
  controlMotor(0, 0, 67); // Drive Motor Left (Channel 0)           
  controlMotor(0, 1, 194); // Drive Motor Right (Channel 1)
  controlMotor(1, 0, 67); // Drive Motor Left (Channel 0)
  controlMotor(1, 1, 194); // Drive Motor Right (Channel 1)
  delay(2000);
  stopmotors();
  delay(1000);
  

}

void Backward(){
   //backward
  controlMotor(0, 0, 194); // Drive Motor Left (Channel 0)           
  controlMotor(0, 1, 67); // Drive Motor Right (Channel 1)str
  controlMotor(1, 0, 194); // Drive Motor Left (Channel 0)
  controlMotor(1, 1, 67); // Drive Motor Right (Channel 1)
  delay(2000);
  stopmotors();
  delay(1000);

}

void CCW(){
  //counter clockwise
  controlMotor(0, 0, 194); // Drive Motor Left (Channel 0)           
  controlMotor(0, 1, 194); // Drive Motor Right (Channel 1)
  controlMotor(1, 0, 194); // Drive Motor Left (Channel 0)
  controlMotor(1, 1, 194); // Drive Motor Right (Channel 1)
  delay(2000);
  stopmotors();
}

void Cw(){
   //clockwise
  controlMotor(0, 0, 67); // Drive Motor Left (Channel 0)           
  controlMotor(0, 1, 67); // Drive Motor Right (Channel 1)
  controlMotor(1, 0, 67); // Drive Motor Left (Channel 0)
  controlMotor(1, 1, 67); // Drive Motor Right (Channel 1)
  delay(2000);
  stopmotors();
  delay(1000);
}

void Left(){
   //strafe left
  controlMotor(0, 0, 67); // Drive Motor Left (Channel 0)           
  controlMotor(0, 1, 127); // Drive Motor Right (Channel 1)
  controlMotor(1, 0, 194); // Drive Motor Left (Channel 0)
  controlMotor(1, 1, 127); // Drive Motor Right (Channel 1)
  delay(2000);
  stopmotors();
  delay(1000);
}

void Right(){
  controlMotor(0, 0, 127); // Drive Motor Left (Channel 0)           
  controlMotor(0, 1, 67); // Drive Motor Right (Channel 1)
  controlMotor(1, 0, 127); // Drive Motor Left (Channel 0)
  controlMotor(1, 1,194); // Drive Motor Right (Channel 1)
  delay(2000);
  stopmotors();
  delay(1000);
}