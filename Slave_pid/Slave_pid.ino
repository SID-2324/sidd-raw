#include <HardwareSerial.h>
#include <PS4Controller.h>
#include <Wire.h>

// ================= I2C =================
#define SDA 21
#define SCL 22
#define SLAVE_ADDR 0x05

// ================= UART =================
#define TX_16 16
#define TX_17 17

#define SER16 Serial2
#define SER17 Serial1

// ================= P CONTROL =================
#define KP 0.6f
#define TARGET_RPM 60.0f
#define MAX_PWM 63

// ================= DATA =================
struct DataPacket {
  float rpm1;
  float rpm2;
  float rpm3;
  float rpm4;
};

// ================= ENUMS =================
enum DIR { CW=0, CCW=1 };
enum CH  { LEFT=0, RIGHT=1 };
enum BUS { BUS16=0, BUS17=1 };

// ================= UART WRITE =================
void motorWrite(BUS b, CH c, DIR d, uint8_t pwm) {
  pwm &= 0x3F;
  uint8_t pkt = (c<<7) | (d<<6) | pwm;
  (b==BUS16 ? SER16 : SER17).write(pkt);
}

// ================= P CONTROLLER =================
uint8_t pwmFromRPM(float tgt, float cur) {
  float out = KP * (tgt - cur);
  if (out < 0) out = -out;
  if (out > MAX_PWM) out = MAX_PWM;
  return (uint8_t)out;
}

// ================= MOTION =================
void stopAll() {
  for(int b=0;b<2;b++)
    for(int c=0;c<2;c++)
      motorWrite((BUS)b,(CH)c,CW,0);
}

void forward(const DataPacket &e) {
  motorWrite(BUS16, LEFT,  CW, pwmFromRPM(TARGET_RPM,e.rpm1));
  motorWrite(BUS16, RIGHT, CW, pwmFromRPM(TARGET_RPM,e.rpm2));
  motorWrite(BUS17, LEFT,  CW, pwmFromRPM(TARGET_RPM,e.rpm3));
  motorWrite(BUS17, RIGHT, CW, pwmFromRPM(TARGET_RPM,e.rpm4));
}

void backward(const DataPacket &e) {
  motorWrite(BUS16, LEFT,  CCW, pwmFromRPM(-TARGET_RPM,e.rpm1));
  motorWrite(BUS16, RIGHT, CCW, pwmFromRPM(-TARGET_RPM,e.rpm2));
  motorWrite(BUS17, LEFT,  CCW, pwmFromRPM(-TARGET_RPM,e.rpm3));
  motorWrite(BUS17, RIGHT, CCW, pwmFromRPM(-TARGET_RPM,e.rpm4));
}

// ================= I2C READ =================
bool readRPM(DataPacket &d) {
  if (Wire.requestFrom(SLAVE_ADDR, sizeof(DataPacket)) != sizeof(DataPacket))
    return false;

  uint8_t *p=(uint8_t*)&d;
  for(uint8_t i=0;i<sizeof(DataPacket);i++) p[i]=Wire.read();
  return true;
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  SER16.begin(115200, SERIAL_8N1, -1, TX_16);
  SER17.begin(115200, SERIAL_8N1, -1, TX_17);

  Wire.begin(SDA, SCL);
  PS4.begin("14:b5:cd:15:c5:5a");

  Serial.println("MASTER READY – 4 MOTOR P CONTROL");
}

// ================= LOOP =================
void loop() {
  if(!PS4.isConnected()){ stopAll(); return; }

  DataPacket enc;
  if(!readRPM(enc)){ stopAll(); return; }

  if(PS4.Circle())      forward(enc);
  else if(PS4.Triangle()) backward(enc);
  else stopAll();

  Serial.printf("RPM: %.1f %.1f %.1f %.1f\n",
    enc.rpm1,enc.rpm2,enc.rpm3,enc.rpm4);

  delay(30);
}