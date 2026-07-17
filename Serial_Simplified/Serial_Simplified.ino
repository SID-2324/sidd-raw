#include <PS4Controller.h>

// Pin Definitions
#define TX1_PIN 17 // MDDS 1 (Front Motors)
#define TX2_PIN 16 // MDDS 2 (Rear Motors)

// Channels for MDDS
#define CH_A 0
#define CH_B 1

// Directions
#define CCW 0
#define CW 1

// --- CONSTANTS ---
const int CONST_SPEED = 10; // Constant speed (Max 63)
const int TURN_SPEED = 35;  // Slightly slower for better control when turning

void setup() {
  Serial.begin(115200);
  
  // Initialize Serial1 for MDDS 1 (Front Left & Front Right)
  Serial1.begin(115200, SERIAL_8N1, -1, TX1_PIN); 
  // Initialize Serial2 for MDDS 2 (Back Left & Back Right)
  Serial2.begin(115200, SERIAL_8N1, -1, TX2_PIN); 

  PS4.begin("18:b5:cd:16:d6:5a");
  Serial.println("Waiting for PS4 connection...");
  
  while (!PS4.isConnected()) {
    delay(250);
  }
  Serial.println("PS4 connected!");
}

void loop() {
  if (PS4.isConnected()) {
    int ly = 0; // Forward/Backward
    int lx = 0; // Strafe
    int rx = 0; // Rotation

    // --- D-PAD LOGIC (Movement) ---
    if (PS4.Up()) {
      ly = CONST_SPEED;
    } else if (PS4.Down()) {
      ly = -CONST_SPEED;
    }

    if (PS4.Left()) {
      lx = -CONST_SPEED;
    } else if (PS4.Right()) {
      lx = CONST_SPEED;
    }

    // --- FACE BUTTONS (Rotation / Extra) ---
    // Using Triangle to turn Left, Circle to turn Right
    if (PS4.Triangle()) {
      rx = -TURN_SPEED; 
    } else if (PS4.Circle()) {
      rx = TURN_SPEED;
    }

    // X-Drive Kinematics
    // Front Left  = Y + X + Rotation
    // Front Right = Y - X - Rotation
    // Back Left   = Y - X + Rotation
    // Back Right  = Y + X - Rotation
    
    int fl = ly + lx + rx;
    int fr = ly - lx - rx;
    int bl = ly - lx + rx;
    int br = ly + lx - rx;

    // Send to motors
    drive(Serial1, CH_A, fl); 
    drive(Serial1, CH_B, fr); 
    drive(Serial2, CH_A, bl); 
    drive(Serial2, CH_B, br); 
  }
  delay(10);
}

// Logic for MDDS Protocol
void motor_write(HardwareSerial &port, uint8_t chan, uint8_t dir, uint8_t speed) {
  uint8_t cmd = 0;
  speed &= 0x3F; // Limit speed to 0-63

  cmd = speed;
  if (chan) cmd |= (1 << 7); 
  if (dir)  cmd |= (1 << 6); 

  port.write(cmd);
}

// Helper to handle direction and limit speed
void drive(HardwareSerial &port, uint8_t chan, int speed) {
  uint8_t dir;
  
  if (speed >= 0) {
    dir = CW; 
  } else {
    dir = CCW;
    speed = -speed;
  }

  // Ensure we never exceed the 0-63 limit of the MDDS protocol
  if (speed > 63) speed = 63;

  motor_write(port, chan, dir, (uint8_t)speed);
}