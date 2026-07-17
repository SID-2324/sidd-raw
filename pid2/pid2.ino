#include <PID_v1_bc.h>

// --- ENCODER SETTINGS ---
#define PPR 230.0           // Your encoder pulses per revolution
#define SAMPLE_TIME 100     // 100ms calculation interval
#define TARGET_RPM 150.0    // Set your desired speed in RPM here

// --- PIN DEFINITIONS ---
#define M1_PWM_PIN  18
#define M1_DIR_PIN  19
#define M1_ENC_A    32
#define M1_ENC_B    33 

// Global Variables
volatile long enc1_counts = 0;
double Setpoint1, Input1, Output1;

// PID Tuning
double Kp=2.0, Ki=1.0, Kd=1.0;

PID PID1(&Input1, &Output1, &Setpoint1, Kp, Ki, Kd, DIRECT);

unsigned long lastTime = 0;
long lastEnc1 = 0;

void IRAM_ATTR readEncoder1() {
  (digitalRead(M1_ENC_B) == LOW) ? enc1_counts++ : enc1_counts--;
}

void setup() {
  Serial.begin(115200);

  // Encoder Pins
  pinMode(M1_ENC_A, INPUT_PULLUP); 
  pinMode(M1_ENC_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(M1_ENC_A), readEncoder1, RISING);

  // Motor Pins
  pinMode(M1_DIR_PIN, OUTPUT);
  ledcAttach(M1_PWM_PIN, 20000, 8);

  // --- CONVERT TARGET RPM TO SETPOINT ---
  Setpoint1 = TARGET_RPM; 

  PID1.SetMode(AUTOMATIC);
  PID1.SetOutputLimits(0, 255);
  PID1.SetSampleTime(SAMPLE_TIME);
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastTime >= SAMPLE_TIME) {
    
    // 1. Get raw pulses in this interval
    long pulses1 = abs(enc1_counts - lastEnc1);

    // 2. CONVERT PULSES TO RPM
    Input1 = (double(pulses1) / PPR) * (60000.0 / SAMPLE_TIME);

    // 3. Compute PID
    PID1.Compute();

    // 4. Drive Motor 1
    digitalWrite(M1_DIR_PIN, HIGH);
    ledcWrite(M1_PWM_PIN, Output1);

    // Debugging
    Serial.print("TargetRPM:"); Serial.print(Setpoint1);
    Serial.print(" CurrentRPM1:"); Serial.print(Input1);
    Serial.print(" PWM_Out:"); Serial.println(Output1);

    lastEnc1 = enc1_counts;
    lastTime = currentTime;
  }
}