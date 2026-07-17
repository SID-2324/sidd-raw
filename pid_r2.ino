
#include <ESP32Encoder.h>
#include <PID_v1.h>
#include <HardwareSerial.h>

// --- CONFIGURATION ---
// Datasheet: Resolution 400 PPR 
// Tutorial Logic: Quadrature Decoding (x4) 
#define ENCODER_PPR 400
#define COUNTS_PER_REV (ENCODER_PPR * 4) // 1600 Counts/Rev

// --- PINS ---
#define PIN_ENC_A 18  // GREEN Wire 
#define PIN_ENC_B 19  // WHITE Wire 
#define MDDS_RX 16    // ESP32 Serial2 RX
#define MDDS_TX 17    // ESP32 Serial2 TX


double Kp = 1.2; 
double Ki = 0.1;
double Kd = 0.05;


double Setpoint, Input, Output;
ESP32Encoder encoder;
// PID Setup: Input=Encoder, Output=Speed, Setpoint=Target
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);
HardwareSerial MDDS_Serial(2);

// --- MOTOR DRIVER HELPER (Packet Serial) ---
void controlMotor(int address, int channel, int pid_output) {
  // Map PID Output (-255 to 255) to MDDS10 Protocol
  // 0 = Full Reverse, 127 = Stop, 255 = Full Forward
  
  int pwm_val = map(pid_output, -255, 255, 0, 255);
  pwm_val = constrain(pwm_val, 0, 255); 

  // Packet Calculation
  byte byte2 = (channel * 8) + address;
  byte checksum = 85 + byte2 + pwm_val;

  MDDS_Serial.write(85);      // Header
  MDDS_Serial.write(byte2);   // Address/Channel
  MDDS_Serial.write(pwm_val); // Speed
  MDDS_Serial.write(checksum);// Checksum
}

void setup() {
  Serial.begin(115200);
  MDDS_Serial.begin(9600, SERIAL_8N1, MDDS_RX, MDDS_TX);

  // --- ENCODER SETUP ---
  // Datasheet Requirement: "This type can be output with internal pull-up resistor" [cite: 37]
  // We enable ESP32 internal pull-ups here:
  ESP32Encoder::useInternalWeakPullResistors = UP; 
  
  encoder.attachHalfQuad(PIN_ENC_A, PIN_ENC_B);
  encoder.setCount(0);

  // --- PID SETUP ---
  Input = 0;
  
  // TARGET: Rotate 5 full revolutions
  // 5 revs * 1600 counts/rev = 8000
  Setpoint = 8000; 

  myPID.SetOutputLimits(-255, 255);
  myPID.SetMode(AUTOMATIC);
  
  delay(1000); // Wait for driver boot
  Serial.println("System Ready. Holding Position: 8000");
}

void loop() {
  // 1. Read Encoder (Replaces 'updateEncoder' from tutorial [cite: 95])
  long newPosition = encoder.getCount();
  Input = (double)newPosition;

  // 2. Calculate PID
  myPID.Compute();

  // 3. Send to Motor (Address 0, Channel 0 -> Left Motor)
  // If motor spins mostly backwards when it should go forward, multiply Output by -1
  controlMotor(0, 0, (int)Output); 

  // 4. Debugging
  // Format: "Target, Current" for Serial Plotter
  Serial.print(Setpoint);
  Serial.print(",");
  Serial.println(Input);
  
  delay(10); // Run at ~100Hz
}