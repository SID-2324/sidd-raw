#include <PS4Controller.h>
#include <AccelStepper.h>

// --- SERIAL PORTS ---
#define MDDS1_Serial Serial2 // Pin 17
#define MDDS2_Serial Serial1 // Pin 16

// --- MOTOR CONSTANTS ---
const int M1_FORWARD = 100, M1_BACKWARD = 30,  M1_STOP = 64;
const int M2_FORWARD = 230, M2_BACKWARD = 150, M2_STOP = 192;
const int M3_OPEN    = 100, M3_CLOSE    = 30,  M3_STOP = 64;

// --- STEPPER CONFIG ---
#define S1 32 
#define D1 18 
#define E1 19 
#define S2 25
#define D2 26 
#define E2 27 

// Math Optimization
const long STEPS_PER_REV = 6400L * 19L; // 121,600 steps
const float SCM = (float)STEPS_PER_REV / (6.0 * 3.14159);
const long MAX_LIMIT = (long)(21.5 * SCM);

AccelStepper m1(1, S1, D1); 
AccelStepper m2(1, S2, D2); 

bool pL1 = false, pR1 = false, pL2 = false, pR2 = false; 

void setup() {
  Serial.begin(115200);

  MDDS1_Serial.begin(9600, SERIAL_8N1, -1, 17);
  MDDS2_Serial.begin(9600, SERIAL_8N1, -1, 16);

  if (!PS4.begin("d0:1b:fb:5a:39:8d")) {
    Serial.println("PS4 Bluetooth Error");
  }

  pinMode(E1, OUTPUT); digitalWrite(E1, LOW); 
  pinMode(E2, OUTPUT); digitalWrite(E2, LOW); 

  // AccelStepper is software-limited. 4000-8000 is a realistic max for ESP32.
  m1.setMaxSpeed(6000); 
  m1.setAcceleration(2000);
  m2.setMaxSpeed(6000); 
  m2.setAcceleration(2000);
  
  Serial.println("System Ready");
}

void loop() {
  if (PS4.isConnected()) {
    
    // --- DC MOTORS (Board 1 & 2) ---
    // Use 'static' to only write to Serial when the state actually changes (prevents lag)
    static int lastM1 = -1, lastM2 = -1, lastM3 = -1;
    int currentM1, currentM2, currentM3;

    // Logic for M1
    if (PS4.Up()) currentM1 = M1_FORWARD;
    else if (PS4.Down()) currentM1 = M1_BACKWARD;
    else currentM1 = M1_STOP;

    // Logic for M2
    if (PS4.Left()) currentM2 = M2_FORWARD;
    else if (PS4.Right()) currentM2 = M2_BACKWARD;
    else currentM2 = M2_STOP;

    // Logic for M3
    if (PS4.Circle()) currentM3 = M3_OPEN;
    else if (PS4.Cross()) currentM3 = M3_CLOSE;
    else currentM3 = M3_STOP;

    // Only write if values changed to save CPU cycles for steppers
    if(currentM1 != lastM1) { MDDS1_Serial.write(currentM1); lastM1 = currentM1; }
    if(currentM2 != lastM2) { MDDS1_Serial.write(currentM2); lastM2 = currentM2; }
    if(currentM3 != lastM3) { MDDS2_Serial.write(currentM3); lastM3 = currentM3; }

    // --- STEPPER 1 (L2 / R2) ---
    if (PS4.L2() && !pL2 && m1.distanceToGo() == 0) {
      long t = m1.currentPosition() + (long)(20 * SCM);
      m1.moveTo(constrain(t, 0, MAX_LIMIT));
    }
    if (PS4.R2() && !pR2 && m1.distanceToGo() == 0) {
      long t = m1.currentPosition() - (long)(20 * SCM);
      m1.moveTo(constrain(t, 0, MAX_LIMIT));
    }

    // --- STEPPER 2 (L1 / R1) ---
    if (PS4.L1() && !pL1 && m2.distanceToGo() == 0) {
      m2.moveTo(m2.currentPosition() + (STEPS_PER_REV / 6));
      Serial.print("move ahead"); // 60 degrees
    }
    if (PS4.R1() && !pR1 && m2.distanceToGo() == 0) {
      m2.moveTo(m2.currentPosition() - (STEPS_PER_REV / 6));
      Serial.print("move back");

    }

    // Update button states
    pL1 = PS4.L1(); pR1 = PS4.R1(); 
    pL2 = PS4.L2(); pR2 = PS4.R2();
  }

  // Essential: Run these every single loop iteration
  m1.run();
  m2.run();
}