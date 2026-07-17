#include <Wire.h>
#include <Arduino.h>

// ================= ENCODER PINS =================
// Encoder 1 (600 PPR)
#define E1A 32
#define E1B 33
#define PPR1 600.0f

// Encoder 2 (600 PPR) – input only pins
#define E2A 34
#define E2B 35
#define PPR2 600.0f

// Encoder 3 (400 PPR)
#define E3A 26
#define E3B 25
#define PPR3 400.0f

// Encoder 4 (400 PPR)  ✅ NEW
#define E4A 18
#define E4B 19
#define PPR4 400.0f

// ================= I2C =================
#define I2C_SDA 21
#define I2C_SCL 22
#define SLAVE_ADDR 0x05

// ================= DATA PACKET =================
struct DataPacket {
  float rpm1;
  float rpm2;
  float rpm3;
  float rpm4;
};

// ================= COUNTERS =================
volatile long c1=0, c2=0, c3=0, c4=0;
long lc1=0, lc2=0, lc3=0, lc4=0;

volatile float r1=0, r2=0, r3=0, r4=0;
unsigned long lastTime = 0;

// ================= ISRs =================
void IRAM_ATTR enc1() { digitalRead(E1B) ? c1-- : c1++; }
void IRAM_ATTR enc2() { digitalRead(E2B) ? c2-- : c2++; }
void IRAM_ATTR enc3() { digitalRead(E3B) ? c3-- : c3++; }
void IRAM_ATTR enc4() { digitalRead(E4B) ? c4-- : c4++; }

// ================= I2C CALLBACK =================
void requestEvent() {
  DataPacket d;
  noInterrupts();
  d.rpm1 = r1;
  d.rpm2 = r2;
  d.rpm3 = r3;
  d.rpm4 = r4;
  interrupts();

  Wire.write((uint8_t*)&d, sizeof(d));
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(E1A, INPUT_PULLUP); pinMode(E1B, INPUT_PULLUP);
  pinMode(E2A, INPUT);        pinMode(E2B, INPUT);
  pinMode(E3A, INPUT_PULLUP); pinMode(E3B, INPUT_PULLUP);
  pinMode(E4A, INPUT_PULLUP); pinMode(E4B, INPUT_PULLUP);

  attachInterrupt(E1A, enc1, RISING);
  attachInterrupt(E2A, enc2, RISING);
  attachInterrupt(E3A, enc3, RISING);
  attachInterrupt(E4A, enc4, RISING);

  Wire.setPins(I2C_SDA, I2C_SCL);
  Wire.begin(SLAVE_ADDR);
  Wire.onRequest(requestEvent);

  lastTime = millis();
  Serial.println("SLAVE READY – 4 ENCODERS ACTIVE");
}

// ================= LOOP =================
void loop() {
  unsigned long now = millis();
  if (now - lastTime >= 100) {

    noInterrupts();
    long nc1=c1, nc2=c2, nc3=c3, nc4=c4;
    interrupts();

    float dt = (now - lastTime);

    r1 = ((nc1 - lc1) / PPR1) * (60000.0f / dt);
    r2 = ((nc2 - lc2) / PPR2) * (60000.0f / dt);
    r3 = ((nc3 - lc3) / PPR3) * (60000.0f / dt);
    r4 = ((nc4 - lc4) / PPR4) * (60000.0f / dt);

    lc1=nc1; lc2=nc2; lc3=nc3; lc4=nc4;
    lastTime = now;
  }
}