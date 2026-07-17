//4 encoders rpm calculation//

const int pin1A = 19; 
const int pin1B = 18; 
const int pin2A = 34; 
const int pin2B = 35; 
const int pin3A = 32; 
const int pin3B = 33; 
const int pin4A = 26; 
const int pin4B = 25; 

long counter1 = 0; 
long counter2 = 0; 
long counter3 = 0; 
long counter4 = 0; 

int lastStateA1;
int lastStateA2;
int lastStateA3;
int lastStateA4;

const float PPR1 = 600.0; 
const float PPR2 = 400.0; 

long lastCounter1 = 0; 
long lastCounter2 = 0; 
long lastCounter3 = 0; 
long lastCounter4 = 0; 

unsigned long lastTime = 0; 
float rpm1 = 0;
float rpm2 = 0;
float rpm3 = 0;
float rpm4 = 0;

void setup() {
  Serial.begin(115200);

  pinMode(pin1A, INPUT_PULLUP);
  pinMode(pin1B, INPUT_PULLUP);
  pinMode(pin2A, INPUT_PULLUP);
  pinMode(pin2B, INPUT_PULLUP);
  pinMode(pin3A, INPUT_PULLUP);
  pinMode(pin3B, INPUT_PULLUP);
  pinMode(pin4A, INPUT_PULLUP);
  pinMode(pin4B, INPUT_PULLUP);

  lastStateA1 = digitalRead(pin1A);
  lastStateA2 = digitalRead(pin2A);
  lastStateA3 = digitalRead(pin3A);
  lastStateA4 = digitalRead(pin4A);
  
  lastTime = millis();
}

void loop() {
  // --- ENCODER 1 ---
  int currentStateA1 = digitalRead(pin1A);
  if (currentStateA1 != lastStateA1) {
    if (digitalRead(pin1B) != currentStateA1) {
      counter1++;
    } else {
      counter1--;
    }
  }
  lastStateA1 = currentStateA1;

  // --- ENCODER 2 ---
  int currentStateA2 = digitalRead(pin2A);
  if (currentStateA2 != lastStateA2) {
    if (digitalRead(pin2B) != currentStateA2) {
      counter2++;
    } else {
      counter2--;
    }
  }
  lastStateA2 = currentStateA2;

  // --- ENCODER 3 ---
  int currentStateA3 = digitalRead(pin3A);
  if (currentStateA3 != lastStateA3) {
    if (digitalRead(pin3B) != currentStateA3) {
      counter3++;
    } else {
      counter3--;
    }
  }
  lastStateA3 = currentStateA3;

  // --- ENCODER 4 ---
  int currentStateA4 = digitalRead(pin4A);
  if (currentStateA4 != lastStateA4) {
    if (digitalRead(pin4B) != currentStateA4) {
      counter4++;
    } else {
      counter4--;
    }
  }
  lastStateA4 = currentStateA4;

  // --- RPM CALCULATION EVERY 100MS ---
  unsigned long currentTime = millis();
  if (currentTime - lastTime >= 100) {
    
    rpm1 = RPM( counter1 , lastCounter1 , PPR1 , currentTime , lastTime );
    rpm2 = RPM( counter2 , lastCounter2 , PPR1 , currentTime , lastTime );
    rpm3 = RPM( counter3 , lastCounter3 , PPR2 , currentTime , lastTime );
    rpm4 = RPM( counter4 , lastCounter4 , PPR2 , currentTime , lastTime );
  
    Serial.print("RPM1: "); Serial.print(rpm1, 1);
    Serial.print(" | RPM2: "); Serial.print(rpm2, 1);
    Serial.print(" | RPM3: "); Serial.print(rpm3, 1);
    Serial.print(" | RPM4: "); Serial.println(rpm4, 1);

    lastCounter1 = counter1;
    lastCounter2 = counter2;
    lastCounter3 = counter3;
    lastCounter4 = counter4;
    lastTime = currentTime;
  }
}

float RPM(long counter , long lastCounter ,float  PPR ,unsigned long currentTime ,unsigned long lastTime){
  long rpm = ((float)(counter - lastCounter) / PPR) * (60000.0 / (currentTime - lastTime));
  return rpm;
}