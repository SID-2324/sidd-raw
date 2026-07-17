#include <PS4Controller.h>
#include <AccelStepper.h>

// MOTORS //

// MDDS uses a single wire for Simplified Serial. 
// We use Serial2 on the ESP32 (Pin 17 is TX2).
#define MDDS_Serial Serial2

// Constants for Fixed Speeds (Range 0-255)
// Motor 1: 64 is Stop, 127 is Full Fwd, 0 is Full Rev
const int M1_FORWARD  = 100; 
const int M1_BACKWARD = 30;
const int M1_STOP     = 64;

// Motor 2: 192 is Stop, 255 is Full Fwd, 128 is Full Rev
const int M2_FORWARD  = 230; 
const int M2_BACKWARD = 150;
const int M2_STOP     = 192;

// Motor 3: 192 is Stop, 255 is Full Fwd, 128 is Full Rev
const int M3_FORWARD  = 230; 
const int M3_BACKWARD = 150;
const int M3_STOP     = 192;

// STEPPER //

// Pins
#define S1 18 // Step 1
#define D1 19 // Dir 1
#define E1 32 // Enable 1
#define S2 27 // Step 2
#define D2 26 // Dir 2
#define E2 25 // Enable 2

#define TX_PIN_1 17
#define TX_PIN_2 12

// Settings
const char* mac = "c0:bf:be:0c:e4:93";
#define S360 (6400 * 19)
#define SCM  ((6400.0 * 19.0) / (6.0 * 3.14159))
#define MAX  (long)(21.5 * SCM)

// Motors
AccelStepper m1(1, S1, D1); 
AccelStepper m2(1, S2, D2); 

// Button States
bool pL1, pR1, pL2, pR2; 

void setup() {
  Serial.begin(115200);

  // Initialize MDDS Serial on UART2 (Baud 9600, TX=17, RX=unused)
  MDDS_Serial1.begin(9600, SERIAL_8N1, -1, 17);
  MDDS_Serial2.begin(9600, SERIAL_8N1, -1, 12);

  // Initialize PS4 with your MAC address
  if (!PS4.begin("a8:ba:69:96:1b:be")) {
    Serial.println("Failed to initialize PS4 Bluetooth");
  }

  Serial.println("Ready! Connect your PS4 controller.");

  pinMode(E1, OUTPUT); digitalWrite(E1, LOW); 
  pinMode(E2, OUTPUT); digitalWrite(E2, LOW); 

  m1.setMaxSpeed(100000); m1.setAcceleration(15000);
  m2.setMaxSpeed(100000); m2.setAcceleration(15000);
}

void loop() {
  // MOTORS //
  if (PS4.isConnected()) {
    
    // --- MOTOR 1 CONTROL (Up/Down Arrows) ---
    if (PS4.Up()) {
      MDDS_Serial1.write(M1_FORWARD);
    } 
    else if (PS4.Down()) {
      MDDS_Serial1.write(M1_BACKWARD);
    } 
    else {
      MDDS_Serial1.write(M1_STOP);
    }

    // --- MOTOR 2 CONTROL (Left/Right Arrows) ---
    if (PS4.Left()) {
      MDDS_Serial1.write(M2_FORWARD);
    } 
    else if (PS4.Right()) {
      MDDS_Serial1.write(M2_BACKWARD);
    } 
    else {
      MDDS_Serial1.write(M2_STOP);
    }
    
     // --- MOTOR 3 CONTROL (Left/Right Arrows) ---
    if (PS4.Circle()) {
      MDDS_Serial2.write(M3_FORWARD);
    } 
    else if (PS4.Triangle()) {
      MDDS_Serial2.write(M3_BACKWARD);
    } 
    else {
      MDDS_Serial2.write(M3_STOP);
    }
  }
  // STEPPER //
  if (PS4.isConnected()) {
    // Motor 1 (Rack)
    if (PS4.L2() && !u && m1.distanceToGo() == 0) {
      long target = m1.currentPosition() + (long)(20 * SCM);
      m1.moveTo(target > MAX ? MAX : target);
    }
    if (PS4.R2() && !d && m1.distanceToGo() == 0) {
      long target = m1.currentPosition() - (long)(20 * SCM);
      m1.moveTo(target < 0 ? 0 : target);
    }

    // Motor 2 (Shaft)
    if (PS4.L1() && !l && m2.distanceToGo() == 0) {
      m2.moveTo(m2.currentPosition() + (long)(60 * S360 / 360));
    }
    if (PS4.R1() && !r && m2.distanceToGo() == 0) {
      m2.moveTo(m2.currentPosition() - (long)(60 * S360 / 360));
    }

    // Save state
    u = PS4.Up(); d = PS4.Down(); l = PS4.L1(); r = PS4.R1();
  }

  m1.run();
  m2.run();
  // Brief delay to prevent flooding the serial buffer
  delay(20);
}