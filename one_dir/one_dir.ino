#include <AccelStepper.h>

// GPIO connections to DM556 driver
#define STEP_PIN      18     // Connects to PUL+
#define DIR_PIN       19     // Connects to DIR+
#define EN_PIN        32     // Connects to ENA+

// --- IMPORTANT: Set this to match your setup ---
// (Motor Steps/Rev * Driver Microsteps)
// Example: 200 steps/rev * 32 microsteps = 6400
// Example: 200 steps/rev * 8 microsteps = 1600
#define STEPS_PER_REV (200 * 32) // Defaulting to 200 steps/rev & 32 microsteps

// NOTE: The user's code had "6400 * 15". 
// This is unusual. '15' might be a gear ratio. 
// If so, (Motor Steps/Rev * Microsteps * Gear Ratio)
// We will use the user's original calculation:
// --- UPDATED This value based on your new code ---
#define STEPS_PER_REV_USER (6400 * 19) 

AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// State variables
int currentAngle = 0;
int stepsToMove = 0;
bool movingForward = true;
bool waiting = false;
unsigned long waitStart = 0;

void setup() {
  // --- UPDATED Baud rate to 9600 ---
  Serial.begin(9600);

  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);  // Enable DM556 (LOW = enable)

  stepper.setMaxSpeed(20*51200); // These high values look like they are from the original code
  stepper.setSpeed(10*51200);    // You may need to tune these
  stepper.setAcceleration(20*25600); // You may need to tune these
  stepper.setCurrentPosition(0);

  // Use the user's specific STEPS_PER_REV definition in serial prints
  Serial.println("Using AccelStepper to 'Move and Return'");
  Serial.printf("STEPS_PER_REV is set to: %ld\n", (long)STEPS_PER_REV_USER);
  Serial.println("✅ Enter angle to rotate (+/-) via Serial Monitor:");
}

void loop() {
  // This is the most important line! It calculates and performs steps.
  stepper.run();

  // Check for new angle input from Serial Monitor
  if (Serial.available() > 0) {
    int angle = Serial.parseInt();
    
    // Clear any remaining characters from the serial buffer
    while(Serial.available()) {
      Serial.read();
    }
    
    if (angle != 0) {
      currentAngle = angle;
      // Calculate steps based on the user's STEPS_PER_REV constant
      stepsToMove = (angle * (float)STEPS_PER_REV_USER) / 360.0;
      Serial.printf("✅ New angle received: %d° (%d steps)\n", currentAngle, stepsToMove);

      // Reset sequence
      stepper.moveTo(stepper.currentPosition() + stepsToMove);
      movingForward = true;
      waiting = false;
    }
  }

  // Continue motion loop only if an angle has been set
  if (currentAngle != 0) {
    // If motion completed and we are not already waiting
    if (!waiting && stepper.distanceToGo() == 0) {
      // --- UPDATED print message for 1 second delay ---
      Serial.printf("...Move complete. Waiting 1 second...\n");
      waitStart = millis();
      waiting = true;
    }

    // --- UPDATED delay time from 3000ms to 1000ms ---
    // Check if the 1-second wait is over
    if (waiting && millis() - waitStart >= 1000) {
      if (movingForward) {
        // Move back to the start
        Serial.println("...Moving back...");
        stepper.moveTo(stepper.currentPosition() - stepsToMove);
      } else {
        // Move forward again
        Serial.println("...Moving forward...");
        stepper.moveTo(stepper.currentPosition() + stepsToMove);
      }
      // Toggle the direction for the next move
      movingForward = !movingForward;
      // We are no longer waiting, we are moving.
      waiting = false;
    }
  }
}

