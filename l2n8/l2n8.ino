#include <PS4Controller.h>

// -------- LEFT SIDE --------
// Front Left
const int FL_EN  = 14;
const int FL_IN1 = 27;
const int FL_IN2 = 26;

// Rear Left
const int RL_EN  = 32;
const int RL_IN3 = 25;
const int RL_IN4 = 33;

// -------- RIGHT SIDE --------
// Front Right
const int FR_EN  = 13;
const int FR_IN1 = 12;
const int FR_IN2 = 15;

// Rear Right
const int RR_EN  = 2;
const int RR_IN3 = 4;
const int RR_IN4 = 16;

// -------- BLADE MOTOR --------
const int BLADE_EN  = 17;
const int BLADE_IN1 = 5;
const int BLADE_IN2 = 18;

bool bladeEnabled = false;

// PWM settings
const int pwmFreq = 1000;
const int pwmResolution = 8;

void setup() {
  Serial.begin(115200);

  // Motor pins
  pinMode(FL_EN, OUTPUT);
  pinMode(FL_IN1, OUTPUT);
  pinMode(FL_IN2, OUTPUT);

  pinMode(RL_EN, OUTPUT);
  pinMode(RL_IN3, OUTPUT);
  pinMode(RL_IN4, OUTPUT);

  pinMode(FR_EN, OUTPUT);
  pinMode(FR_IN1, OUTPUT);
  pinMode(FR_IN2, OUTPUT);

  pinMode(RR_EN, OUTPUT);
  pinMode(RR_IN3, OUTPUT);
  pinMode(RR_IN4, OUTPUT);

  // Enable drive motors
  digitalWrite(FL_EN, HIGH);
  digitalWrite(RL_EN, HIGH);
  digitalWrite(FR_EN, HIGH);
  digitalWrite(RR_EN, HIGH);

  // Blade motor setup
  pinMode(BLADE_IN1, OUTPUT);
  pinMode(BLADE_IN2, OUTPUT);

  digitalWrite(BLADE_IN1, HIGH);
  digitalWrite(BLADE_IN2, LOW);

  // PWM (ESP32 v3)
  ledcAttach(BLADE_EN, pwmFreq, pwmResolution);

  // Start PS4
  PS4.begin("18:B5:CD:16:D6:5A");
  Serial.println("READY: Connect PS4");
}

void loop() {

  if (PS4.isConnected()) {

    // -------- MOVEMENT --------

    if (PS4.Up()) {
      digitalWrite(FL_IN1, HIGH); digitalWrite(FL_IN2, LOW);
      digitalWrite(RL_IN3, HIGH); digitalWrite(RL_IN4, LOW);

      digitalWrite(FR_IN1, HIGH); digitalWrite(FR_IN2, LOW);
      digitalWrite(RR_IN3, HIGH); digitalWrite(RR_IN4, LOW);
    }

    else if (PS4.Down()) {
      digitalWrite(FL_IN1, LOW); digitalWrite(FL_IN2, HIGH);
      digitalWrite(RL_IN3, LOW); digitalWrite(RL_IN4, HIGH);

      digitalWrite(FR_IN1, LOW); digitalWrite(FR_IN2, HIGH);
      digitalWrite(RR_IN3, LOW); digitalWrite(RR_IN4, HIGH);
    }

    else if (PS4.Left()) {
      digitalWrite(FL_IN1, LOW); digitalWrite(FL_IN2, HIGH);
      digitalWrite(RL_IN3, LOW); digitalWrite(RL_IN4, HIGH);

      digitalWrite(FR_IN1, HIGH); digitalWrite(FR_IN2, LOW);
      digitalWrite(RR_IN3, HIGH); digitalWrite(RR_IN4, LOW);
    }

    else if (PS4.Right()) {
      digitalWrite(FL_IN1, HIGH); digitalWrite(FL_IN2, LOW);
      digitalWrite(RL_IN3, HIGH); digitalWrite(RL_IN4, LOW);

      digitalWrite(FR_IN1, LOW); digitalWrite(FR_IN2, HIGH);
      digitalWrite(RR_IN3, LOW); digitalWrite(RR_IN4, HIGH);
    }

    else {
      digitalWrite(FL_IN1, LOW); digitalWrite(FL_IN2, LOW);
      digitalWrite(RL_IN3, LOW); digitalWrite(RL_IN4, LOW);

      digitalWrite(FR_IN1, LOW); digitalWrite(FR_IN2, LOW);
      digitalWrite(RR_IN3, LOW); digitalWrite(RR_IN4, LOW);
    }

    // -------- BLADE CONTROL (IMPROVED) --------

    // ❌ X pressed → turn ON
    if (PS4.event.button_down.cross) {
      bladeEnabled = true;
    }

    // ⭕ O pressed → turn OFF
    if (PS4.event.button_down.circle) {
      bladeEnabled = false;
    }

    // Speed control
    int bladeSpeed = 0;

    if (bladeEnabled) {
      bladeSpeed = PS4.R2Value();
    }

    ledcWrite(BLADE_EN, bladeSpeed);

    // Debug
    Serial.print("Blade: ");
    Serial.print(bladeEnabled ? "ON" : "OFF");
    Serial.print(" | Speed: ");
    Serial.println(bladeSpeed);
  }
}