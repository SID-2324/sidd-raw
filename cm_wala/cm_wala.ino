#include <AccelStepper.h>

// GPIO connections to TB6600/DM556 driver
#define STEP_PIN1      18    
#define DIR_PIN1       19      
#define EN_PIN1        32  

#define STEP_PIN2      25     
#define DIR_PIN2       26      
#define EN_PIN2        27  

// --- PHYSICAL SYSTEM CONSTANTS ---
#define STEPS_PER_REV_360     (6400 * 19)      // 121,600 steps per 360 degree revolution
#define PINION_DIAMETER_CM1    6.0        
#define PINION_DIAMETER_CM2    4.0         
#define RACK_LENGTH_CM        100             
#define PI_CONST              3.1415926535

// Calculated steps per 1 cm of linear travel
const float STEPS_PER_CM1 = (float)STEPS_PER_REV_360 / (PINION_DIAMETER_CM1 * PI_CONST);
const float STEPS_PER_CM2 = (float)STEPS_PER_REV_360 / (PINION_DIAMETER_CM2 * PI_CONST);

// --- DEFAULT SETTINGS ---
#define DEFAULT_MAX_SPEED     (100 * 1000)     
#define DEFAULT_ACCELERATION  (10 * 1000) 

// Initialize two stepper instances
AccelStepper stepper1(AccelStepper::DRIVER, STEP_PIN1, DIR_PIN1);
AccelStepper stepper2(AccelStepper::DRIVER, STEP_PIN2, DIR_PIN2);

float currentAcceleration = DEFAULT_ACCELERATION;

// --- RACK BOUNDING ---
long maxSteps1 = 0; 
long maxSteps2 = 0; 

void setup() {
  Serial.begin(9600);
  
  // Calculate maximum steps for each motor based on rack length
  maxSteps1 = (long)(RACK_LENGTH_CM * STEPS_PER_CM1);
  maxSteps2 = (long)(RACK_LENGTH_CM * STEPS_PER_CM2);

  // --- Hardware Setup ---
  pinMode(EN_PIN1, OUTPUT);
  digitalWrite(EN_PIN1, LOW);  
  
  pinMode(EN_PIN2, OUTPUT);
  digitalWrite(EN_PIN2, LOW); 

  // --- AccelStepper 1 Setup ---
  stepper1.setMaxSpeed(DEFAULT_MAX_SPEED); 
  stepper1.setAcceleration(DEFAULT_ACCELERATION);
  stepper1.setCurrentPosition(0); 

  // --- AccelStepper 2 Setup ---
  stepper2.setMaxSpeed(DEFAULT_MAX_SPEED); 
  stepper2.setAcceleration(DEFAULT_ACCELERATION);
  stepper2.setCurrentPosition(0); 

  Serial.println("✅ Dual Rack & Pinion Control Ready.");
  Serial.printf("STEPS_PER_CM: M1=%.2f, M2=%.2f\n", STEPS_PER_CM1, STEPS_PER_CM2);
  Serial.println("Enter commands: A=Acc or D=Dist (e.g., D=5.0)");
}

void loop() {
  // 1. Motor Engines (Run both every loop)
  stepper1.run();
  stepper2.run();

  // 2. Serial Input Handler
  if (Serial.available()) {
    handleSerialInput();
  }
}

void handleSerialInput() {
  String input = Serial.readStringUntil('\n'); 
  input.trim(); 

  if (input.length() > 0) {
    char command = input.charAt(0);
    int equalsIndex = input.indexOf('=');
    
    if (equalsIndex == -1 || input.length() <= equalsIndex + 1) {
      Serial.println("🚫 Invalid format. Use A=value or D=value.");
      return;
    }

    String valueString = input.substring(equalsIndex + 1);
    
    if (command == 'A' || command == 'a') {
      float newAcc = valueString.toFloat();
      if (newAcc > 0) {
        currentAcceleration = newAcc;
        stepper1.setAcceleration(currentAcceleration);
        stepper2.setAcceleration(currentAcceleration);
        Serial.printf("✅ Acceleration set to: %.0f\n", currentAcceleration);
      }
      
    } else if (command == 'D' || command == 'd') {
      float distanceCM = valueString.toFloat();
      
      // Calculate individual targets based on different pinion diameters
      long target1 = stepper1.currentPosition() + (long)(distanceCM * STEPS_PER_CM1);
      long target2 = stepper2.currentPosition() + (long)(distanceCM * STEPS_PER_CM2);

      // Bounding Check Motor 1
      if (target1 < 0) target1 = 0;
      else if (target1 > maxSteps1) target1 = maxSteps1;

      // Bounding Check Motor 2
      if (target2 < 0) target2 = 0;
      else if (target2 > maxSteps2) target2 = maxSteps2;
      
      // Move both to their calculated absolute positions
      stepper1.moveTo(target1);
      stepper2.moveTo(target2);


    }
  }
}