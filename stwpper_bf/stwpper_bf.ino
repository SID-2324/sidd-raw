#include <AccelStepper.h>

// GPIO connections to DM556 driver
#define STEP_PIN     14     // Connects to PUL+
#define DIR_PIN      12    // Connects to DIR+
#define EN_PIN       16    // Connects to ENA+

#define STEPS_PER_REV 6400 * 15  // 200 steps/rev × 32 microsteps

AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// State variables
int currentAngle = 0;
int stepsToMove = 0;
bool movingForward = true;
bool waiting = false;
unsigned long waitStart = 0;

void setup() {
  Serial.begin(115200);

  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);  // Enable DM556 (LOW = enable)

  stepper.setMaxSpeed(20*51200);
  stepper.setSpeed(10*51200);
  stepper.setAcceleration(20*25600);
  stepper.setCurrentPosition(0);

  Serial.println("✅ Enter angle to rotate (+/-):");
}

void loop() {
  stepper.run();

  // Check for new angle input
  if (Serial.available() > 0) {
    int angle = Serial.parseInt();
    if (angle != 0) {
      currentAngle = angle;
      stepsToMove = (angle * STEPS_PER_REV) / 360;
      Serial.printf("✅ New angle received: %d° (%d steps)\n", currentAngle, stepsToMove);

      // Reset sequence
      stepper.moveTo(stepper.currentPosition() + stepsToMove);
      movingForward = true;
      waiting = false;
    }
  }

  // Continue motion loop if angle is set
  if (currentAngle != 0) {
    // If motion completed and not waiting
    if (!waiting && stepper.distanceToGo() == 0) {
      Serial.printf("⏳ Waiting 3 seconds...\n");
      waitStart = millis();
      waiting = true;
    }

    // ⬅️⬅️ Changed this line from 5000 to 3000 for 3-second delay
    if (waiting && millis() - waitStart >= 3000) {
      if (movingForward) {
        stepper.moveTo(stepper.currentPosition() - stepsToMove);
      } else {
        stepper.moveTo(stepper.currentPosition() + stepsToMove);
      }
      movingForward = !movingForward;
      waiting = false;
    }
  }
}