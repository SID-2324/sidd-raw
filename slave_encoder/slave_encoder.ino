#include <driver/spi_slave.h>

// ENCODER PINS 
const int pin1A = 32; 
const int pin1B = 33; 
const int pin2A = 34; 
const int pin2B = 35; 
const int pin3A = 26; 
const int pin3B = 25; 
const int pin4A = 22; 
const int pin4B = 21;

// SPI PINS (HSPI)
#define GPIO_MOSI 13
#define GPIO_MISO 12
#define GPIO_SCLK 14
#define GPIO_CS 15

// VARIABLES 
volatile long counter1 = 0; 
volatile long counter2 = 0; 
volatile long counter3 = 0; 
volatile long counter4 = 0; 

int lastStateA1, lastStateA2, lastStateA3, lastStateA4;

const float PPR1 = 600.0;  // Encoders 1 & 2
const float PPR2 = 400.0;  // Encoders 3 & 4

long lastCounter1 = 0; 
long lastCounter2 = 0; 
long lastCounter3 = 0; 
long lastCounter4 = 0; 

unsigned long lastTime = 0; 

float rpm1 = 0, rpm2 = 0, rpm3 = 0, rpm4 = 0;

// SPI PACKET
struct __attribute__((packed)) EncoderData {
  int16_t rpm1;
  int16_t rpm2;
  int16_t rpm3;
  int16_t rpm4;
};

WORD_ALIGNED_ATTR uint8_t txBuffer[sizeof(EncoderData)];
WORD_ALIGNED_ATTR uint8_t rxBuffer[sizeof(EncoderData)];

spi_slave_transaction_t trans;

void setupSPISlave() {
  // SPI slave configuration
  spi_bus_config_t buscfg = {
    .mosi_io_num = GPIO_MOSI,
    .miso_io_num = GPIO_MISO,
    .sclk_io_num = GPIO_SCLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = sizeof(EncoderData),
  };

  spi_slave_interface_config_t slvcfg = {
    .spics_io_num = GPIO_CS,
    .flags = 0,
    .queue_size = 3,
    .mode = 0,
    .post_setup_cb = NULL,
    .post_trans_cb = NULL
  };

  // Initialize SPI slave
  gpio_set_pull_mode((gpio_num_t)GPIO_MOSI, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode((gpio_num_t)GPIO_SCLK, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode((gpio_num_t)GPIO_CS, GPIO_PULLUP_ONLY);

  esp_err_t ret = spi_slave_initialize(HSPI_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK) {
    Serial.println("SPI Slave init failed!");
  } else {
    Serial.println("SPI Slave initialized successfully");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Pin setup
  pinMode(pin1A, INPUT_PULLUP);
  pinMode(pin1B, INPUT_PULLUP);
  pinMode(pin2A, INPUT_PULLUP);
  pinMode(pin2B, INPUT_PULLUP);
  pinMode(pin3A, INPUT_PULLUP);
  pinMode(pin3B, INPUT_PULLUP);
  pinMode(pin4A, INPUT_PULLUP);
  pinMode(pin4B, INPUT_PULLUP);

  // Initial states
  lastStateA1 = digitalRead(pin1A);
  lastStateA2 = digitalRead(pin2A);
  lastStateA3 = digitalRead(pin3A);
  lastStateA4 = digitalRead(pin4A);

  lastTime = millis();

  // Setup SPI Slave
  setupSPISlave();
  
  Serial.println("ESP32 SPI Encoder Slave Ready");
  Serial.println("HSPI - SCLK:14, MISO:12, MOSI:13, SS:15");
}

void updateSPIBuffer() {
  EncoderData data;
  data.rpm1 = (int16_t)rpm1;
  data.rpm2 = (int16_t)rpm2;
  data.rpm3 = (int16_t)rpm3;
  data.rpm4 = (int16_t)rpm4;
  
  memcpy(txBuffer, &data, sizeof(EncoderData));
  
  // Prepare transaction
  memset(&trans, 0, sizeof(trans));
  trans.length = sizeof(EncoderData) * 8;  // in bits
  trans.tx_buffer = txBuffer;
  trans.rx_buffer = rxBuffer;
  
  // Queue transaction (non-blocking)
  spi_slave_queue_trans(HSPI_HOST, &trans, 0);
}

void loop() {
  // Encoder polling
  int currentStateA1 = digitalRead(pin1A);
  if (currentStateA1 != lastStateA1) {
    if (digitalRead(pin1B) != currentStateA1) counter1++;
    else counter1--;
  }
  lastStateA1 = currentStateA1;

  int currentStateA2 = digitalRead(pin2A);
  if (currentStateA2 != lastStateA2) {
    if (digitalRead(pin2B) != currentStateA2) counter2++;
    else counter2--;
  }
  lastStateA2 = currentStateA2;

  int currentStateA3 = digitalRead(pin3A);
  if (currentStateA3 != lastStateA3) {
    if (digitalRead(pin3B) != currentStateA3) counter3++;
    else counter3--;
  }
  lastStateA3 = currentStateA3;

  int currentStateA4 = digitalRead(pin4A);
  if (currentStateA4 != lastStateA4) {
    if (digitalRead(pin4B) != currentStateA4) counter4++;
    else counter4--;
  }
  lastStateA4 = currentStateA4;

  // RPM calculation every 100ms
  unsigned long currentTime = millis();
  if (currentTime - lastTime >= 100) {
    // Calculate RPMs
    rpm1 = ((float)(counter1 - lastCounter1) / PPR1) * (60000.0 / (currentTime - lastTime));
    rpm2 = ((float)(counter2 - lastCounter2) / PPR1) * (60000.0 / (currentTime - lastTime));
    rpm3 = ((float)(counter3 - lastCounter3) / PPR2) * (60000.0 / (currentTime - lastTime));
    rpm4 = ((float)(counter4 - lastCounter4) / PPR2) * (60000.0 / (currentTime - lastTime));

    // Update SPI buffer
    updateSPIBuffer();

    // Update counters and time
    lastCounter1 = counter1;
    lastCounter2 = counter2;
    lastCounter3 = counter3;
    lastCounter4 = counter4;
    lastTime = currentTime;

    // Debug output
    Serial.print("RPM: ");
    Serial.print((int16_t)rpm1); Serial.print(" | ");
    Serial.print((int16_t)rpm2); Serial.print(" | ");
    Serial.print((int16_t)rpm3); Serial.print(" | ");
    Serial.println((int16_t)rpm4);
  }
}