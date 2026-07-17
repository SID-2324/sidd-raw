#include <PS4Controller.h>
#include <AccelStepper.h>

// --- STEPPER PINS ---
#define S1 32 
#define D1 18 
#define E1 19 
#define S2 25 
#define D2 26 
#define E2 27 

// --- STEPPER MATH ---
const char* mac = "a8:ba:69:96:1b:be";
#define S360 (6400L * 19L) // Added 'L' to ensure long math
#define SCM  ((6400.0 * 19.0) / (6.0 * 3.14159))
#define MAX  (long)(21.5 * SCM)

AccelStepper m1(1, S1, D1); 
AccelStepper m2(1, S2, D2); 

// --- STATE TRACKING ---
bool pL1, pR1, pL2, pR2; 

void setup() {
  Serial.begin(115200);
  PS4.begin(mac); 

  pinMode(E1, OUTPUT); digitalWrite(E1, LOW); // Enable motors
  pinMode(E2, OUTPUT); digitalWrite(E2, LOW); 

  m1.setMaxSpeed(100000); m1.setAcceleration(15000);
  m2.setMaxSpeed(100000); m2.setAcceleration(15000);

  Serial.println("Stepper Control Ready.");
}

void loop() {
  if (PS4.isConnected()) {
    
    // 1. STEPPER 1 (Rack - L2/R2)
    // Moves 20cm per press, bounded by 0 and MAX
    if (PS4.L2() && !pL2 && m1.distanceToGo() == 0) {
      long t = m1.currentPosition() + (long)(20 * SCM);
      m1.moveTo(constrain(t, 0, MAX)); // Syntax fix: actually call moveTo
    }
    if (PS4.R2() && !pR2 && m1.distanceToGo() == 0) {
      long t = m1.currentPosition() - (long)(20 * SCM);
      m1.moveTo(constrain(t, 0, MAX)); // Syntax fix: actually call moveTo
    }

    // 2. STEPPER 2 (Shaft - L1/R1)
    // Rotates 60 degrees per press
    if (PS4.L1() && !pL1 && m2.distanceToGo() == 0) {
      m2.moveTo(m2.currentPosition() + (60L * S360 / 360L));
    }
    if (PS4.R1() && !pR1 && m2.distanceToGo() == 0) {
      m2.moveTo(m2.currentPosition() - (60L * S360 / 360L));
    }

    // Update previous states
    pL1 = PS4.L1(); pR1 = PS4.R1(); 
    pL2 = PS4.L2(); pR2 = PS4.R2();
  }

  // Update motor positions (Must be outside the PS4.isConnected block for smooth motion)
  m1.run();
  m2.run();
}