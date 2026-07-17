#include <I2C_MPU6886.h>
#include <Wire.h>

I2C_MPU6886 imu(I2C_MPU6886_DEFAULT_ADDRESS, Wire);

// --- Rolling Average Settings ---
#define WINDOW_SIZE 20  
float gyroX_window[WINDOW_SIZE], gyroY_window[WINDOW_SIZE], gyroZ_window[WINDOW_SIZE];
float accelX_window[WINDOW_SIZE], accelY_window[WINDOW_SIZE], accelZ_window[WINDOW_SIZE];
int readIndex = 0;

// Offsets for Gyro only
float gx_offset = 0, gy_offset = 0, gz_offset = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  imu.begin();

  // 1. Initial Deep Calibration for Gyro
  Serial.println("INITIALIZING: Keep still for deep calibration...");
  float sumGX = 0, sumGY = 0, sumGZ = 0;
  int calSamples = 2000; 
  for (int i = 0; i < calSamples; i++) {
    float gx, gy, gz;
    imu.getGyro(&gx, &gy, &gz);
    sumGX += gx; sumGY += gy; sumGZ += gz;
    if(i % 500 == 0) Serial.print(".");
    delay(2);
  }
  gx_offset = sumGX / (float)calSamples;
  gy_offset = sumGY / (float)calSamples;
  gz_offset = sumGZ / (float)calSamples;
  
  // Initialize all windows with 0
  for (int i = 0; i < WINDOW_SIZE; i++) {
    gyroX_window[i] = 0; gyroY_window[i] = 0; gyroZ_window[i] = 0;
    accelX_window[i] = 0; accelY_window[i] = 0; accelZ_window[i] = 0;
  }

  Serial.println("\nCalibration Done. Starting Multi-Sensor Stream...");
}

void loop() {
  float gx_raw, gy_raw, gz_raw;
  float ax_raw, ay_raw, az_raw;
  
  imu.getGyro(&gx_raw, &gy_raw, &gz_raw);
  imu.getAccel(&ax_raw, &ay_raw, &az_raw);

  // 2. Update Rolling Windows
  // Gyro gets offset correction, Accel is stored raw
  gyroX_window[readIndex] = gx_raw - gx_offset;
  gyroY_window[readIndex] = gy_raw - gy_offset;
  gyroZ_window[readIndex] = gz_raw - gz_offset;
  
  accelX_window[readIndex] = ax_raw;
  accelY_window[readIndex] = ay_raw;
  accelZ_window[readIndex] = az_raw;

  // 3. Calculate Averages
  float avgGX = 0, avgGY = 0, avgGZ = 0;
  float avgAX = 0, avgAY = 0, avgAZ = 0;
  
  for (int i = 0; i < WINDOW_SIZE; i++) {
    avgGX += gyroX_window[i]; avgGY += gyroY_window[i]; avgGZ += gyroZ_window[i];
    avgAX += accelX_window[i]; avgAY += accelY_window[i]; avgAZ += accelZ_window[i];
  }
  
  avgGX /= WINDOW_SIZE; avgGY /= WINDOW_SIZE; avgGZ /= WINDOW_SIZE;
  avgAX /= WINDOW_SIZE; avgAY /= WINDOW_SIZE; avgAZ /= WINDOW_SIZE;

  // 4. Advance Index
  readIndex = (readIndex + 1) % WINDOW_SIZE;

  // 5. Output Data
  // Printing Gyro (Degrees per second) and Accel (G-force)
  Serial.printf("GYRO | X:%.2f Y:%.2f Z:%.2f  ", avgGX, avgGY, avgGZ);
  Serial.printf("ACCEL | X:%.2f Y:%.2f Z:%.2f\n", avgAX, avgAY, avgAZ);

  delay(50); 
}
