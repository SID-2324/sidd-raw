// Define the GPIO pins for the encoder's A and B channels
#define ENCODER_A_PIN 4
#define ENCODER_B_PIN 5

// Pulses Per Revolution for your specific encoder
#define PPR 600

// Use 'volatile' for any variable that is modified inside an interrupt routine
volatile long pulse_count = 0;

// Variables for the RPM calculation
unsigned long previous_time = 0;
long previous_pulse_count = 0;
const int calculation_interval_ms = 500; // Calculate RPM every 500ms

// --- Interrupt Service Routine (ISR) ---
// This function is called automatically every time Channel A has a rising edge.
// IRAM_ATTR helps it run faster from the ESP32's internal RAM.
void IRAM_ATTR readEncoder() {
  // Check the state of Channel B to determine the direction of rotation
  if (digitalRead(ENCODER_A_PIN) == digitalRead(ENCODER_B_PIN)) {
    pulse_count++; // Turning Clockwise
  } else {
    pulse_count--; // Turning Counter-Clockwise
  }
}

void setup() {
  // Start serial communication to print the output
  Serial.begin(115200);

  // Set the encoder pins as inputs
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);

  // Attach the interrupt to the Channel A pin
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), readEncoder, RISING);

  // Record the starting time for our first calculation
  previous_time = millis();
  
  Serial.println("RPM Calculator Initialized...");
}

void loop() {
  // Get the current time at the start of the loop
  unsigned long current_time = millis();

  // Check if our desired interval has passed since the last calculation
  if (current_time - previous_time >= calculation_interval_ms) {
    
    // --- Safely read the pulse count ---
    // We temporarily disable interrupts to make sure the pulse_count variable
    // isn't changed while we are in the middle of reading it.
    noInterrupts();
    long current_pulse_count = pulse_count;
    interrupts(); // Re-enable interrupts immediately after
    // ------------------------------------

    // Calculate how many pulses have occurred since the last check
    long pulse_delta = current_pulse_count - previous_pulse_count;

    // Calculate the time that has passed in seconds
    float time_delta_seconds = (current_time - previous_time) / 1000.0;

    // The RPM formula:
    // (Pulses / Second) * (60 Seconds / Minute) / (Pulses / Revolution) = Revolutions / Minute
    float rpm = ( ( (float)pulse_delta / time_delta_seconds ) * 60.0 ) / (float)PPR;

    // Print the final RPM value. We use abs() to show speed as a positive number.
    Serial.print("Current RPM: ");
    Serial.println(abs(rpm));

    // Update our "previous" variables for the next cycle
    previous_time = current_time;
    previous_pulse_count = current_pulse_count;
  }
}