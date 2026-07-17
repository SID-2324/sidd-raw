// Define the ESP32 pins you connected to the driver
const int DIR_PIN = 12; // Direction Pin
const int PUL_PIN = 14; // Pulse Pin
// 2000 = Slow Speed
// 1000 = Medium Speed
// 500  = Fast Speed
int pulseDelay = 2000; // Delay in microseconds (us)

void setup() {
  // Set the pins as Outputs
  pinMode(DIR_PIN, OUTPUT);
  pinMode(PUL_PIN, OUTPUT);

  // Set the direction.
  // HIGH = one direction, LOW = the other direction.
  digitalWrite(DIR_PIN, HIGH);
}

void loop() {
  
    digitalWrite(DIR_PIN, HIGH);
  for(int i=2000 ; i>= 400 ; i--){
     pulseDelay = i;
     pulse();
  }
    digitalWrite(DIR_PIN, LOW);
  for(int i=400 ; i>= 2000 ; i++){
     pulseDelay = i;
     pulse();
  }
}

void pulse(){
  digitalWrite(PUL_PIN, HIGH);
  // 2. Wait for half the delay
  delayMicroseconds(pulseDelay);
  
  // 3. Send the pulse LOW
  digitalWrite(PUL_PIN, LOW);
  // 4. Wait for the other half of the delay
  delayMicroseconds(pulseDelay);

  
}
