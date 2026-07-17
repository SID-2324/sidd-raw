// Define the pins for the three ultrasonic sensors
const int trigPin1 = 27;
const int echoPin1 = 26;

const int trigPin2 = 25;
const int echoPin2 = 33;

const int trigPin3 = 32;
const int echoPin3 = 35;

// Variables to store duration and distance for each sensor
long duration1, duration2, duration3;
int distanceCm1, distanceCm2, distanceCm3;

void setup() {
  Serial.begin(9600); // Start serial communication

  // Set up pins for Sensor 1
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);

  // Set up pins for Sensor 2
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  // Set up pins for Sensor 3
  pinMode(trigPin3, OUTPUT);
  pinMode(echoPin3, INPUT);
}

// Function to get distance from a single sensor
int getDistance(int trigPin, int echoPin) {
  // Clear the Trig pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Send a 10-microsecond pulse
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the echo pulse duration
  long duration = pulseIn(echoPin, HIGH);

  // Calculate distance
  return duration * 0.034 / 2;
}

void loop() {
  // Get distance from each sensor
  distanceCm1 = getDistance(trigPin1, echoPin1);
  distanceCm2 = getDistance(trigPin2, echoPin2);
  distanceCm3 = getDistance(trigPin3, echoPin3);

  // Calculate the average distance
  int avgDistance = (distanceCm1 + distanceCm2 + distanceCm3) / 1;

  Serial.print("Average Distance: ");
  Serial.print(avgDistance);
  Serial.println(" cm");
  
  delay(100); // Wait for a second before the next reading
}