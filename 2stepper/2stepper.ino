#include <AccelStepper.h>

// GPIO connections to DM556 driver
#define STEP_PIN      14      // Connects to PUL+
#define DIR_PIN       12      // Connects to DIR+
#define EN_PIN        16      // Connects to ENA+

// --- STEPS CONSTANT ---
// (Motor Steps/Rev * Driver Microsteps * Gear Ratio)
// Value from your original code: 6400 * 19 = 121,600 steps per 360 degrees
#define STEPS_PER_REV_USER (6400 * 19)

// --- TARGET ANGLE ---
// We define the fixed angle here, so the code only runs this one move.
#define TARGET_ANGLE_DEGREE 90

AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// Calculate the fixed number of steps required for the target angle
long stepsToMove = 0;

void setup() {
  Serial.begin(9600);
  
  // --- Hardware Setup ---
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);  // Enable DM556 (LOW = enable)

  // --- AccelStepper Setup ---
  stepper.setMaxSpeed(20 * 51200); 
  stepper.setSpeed(10 * 51200);   
  stepper.setAcceleration(20 * 25600);
  stepper.setCurrentPosition(0); // Start position is 0

  // --- Calculation ---
  // Formula: steps = (angle / 360.0) * steps_per_360_degrees
  stepsToMove = (long)((TARGET_ANGLE_DEGREE * (float)STEPS_PER_REV_USER) / 360.0);

  Serial.println("Using AccelStepper for a single, fixed move.");
  Serial.printf("Target Angle: %d°\n", TARGET_ANGLE_DEGREE);
  Serial.printf("Steps to Move: %ld\n", stepsToMove);
  
  // --- Command the Move ---
  // Tell the motor to move from 0 to the calculated step count.
  stepper.moveTo(stepsToMove);
}

void loop() {
  // This single line executes the movement (acceleration, running, deceleration)
  stepper.run();

  // Check if the motor has reached its target position
  if (stepper.distanceToGo() == 0) {
    // If we've reached the target, we print a message once and stop running.
    // The motor holds position due to the enabled driver.
    if (stepsToMove != -1) { // Use -1 as a flag to prevent repeat printing
      Serial.printf("✅ Movement complete. Stopped at position: %ld steps\n", stepper.currentPosition());
      stepsToMove = -1; 
      // Note: Since we don't call moveTo() again, the motor will stay still.
    }
    // You could disable the motor here to save power:
    // digitalWrite(EN_PIN, HIGH); // HIGH = disable
  }
}