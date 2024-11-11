#include <Wire.h>

// Pin definitions
#define SENSOR_PIN_1 A0
#define SENSOR_PIN_2 A1
#define SENSOR_PIN_3 A2
#define SENSOR_PIN_4 A3
#define BUTTON_PIN_WRITE 2

// EEPROM I2C address
#define EEPROM_ADDRESS 0x50

// Variables to manage writing
bool writing = false;
bool lastButtonState = false;
unsigned int writeAddress = 3; // Start writing from address 3
unsigned long lastWriteMillis = 0;
bool sensorStates[4] = {false, false, false, false}; // Track which sensors are enabled

// Function prototypes
void writeEEPROM(unsigned int addr, unsigned int value);
unsigned int readEEPROM(unsigned int addr);
void sendEEPROMData();
void processCommand(char command);
void writeSensorStatesToEEPROM();
void readSensorStatesFromEEPROM();
void handleButtonPress();
void writeSensorData();

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  pinMode(BUTTON_PIN_WRITE, INPUT_PULLUP); // Enable internal pull-up resistor

  // Retrieve sensor states from EEPROM
  readSensorStatesFromEEPROM();
}

void loop() {
  handleButtonPress();

  // Writing logic
  if (writing && millis() - lastWriteMillis >= 1) {
    writeSensorData();
  }

  if (Serial.available()) {
    char command = Serial.read();
    if (command == 'R') {
      sendEEPROMData();
    } else {
      processCommand(command);
    }
  }
}

void handleButtonPress() {
  bool currentButtonState = digitalRead(BUTTON_PIN_WRITE);

  // Check if the button state has changed
  if (currentButtonState != lastButtonState) {
    if (currentButtonState == LOW) {
      writing = !writing;
      writeAddress = 3; // Reset address when starting to write
      lastWriteMillis = millis();
      Serial.println(writing ? "Writing started" : "Writing stopped");
    }
  }

  lastButtonState = currentButtonState;
}

void writeSensorData() {
  lastWriteMillis = millis();
    
  // Write enabled sensor values
  for (int i = 0; i < 4; i++) {
    if (sensorStates[i]) {
      int sensorPin = SENSOR_PIN_1 + i;
      int sensorValue = analogRead(sensorPin); // Read analog sensor
      writeEEPROM(writeAddress, sensorValue); // Write to EEPROM
      Serial.print("Writing - Address: ");
      Serial.print(writeAddress);
      Serial.print(", Sensor value: ");
      Serial.println(sensorValue);
      
      writeAddress += 2; // Move to the next address for the next write (2 bytes per value)
    }
  }

  // Update last written address
  writeEEPROM(0, writeAddress - 1); // Store last written address to EEPROM address 0

  // Prevent address overflow
  if (writeAddress >= 32767) { // Adjust to your EEPROM size
    writing = false; // Stop writing if EEPROM is full
    Serial.println("EEPROM full");
  }
}

void writeEEPROM(unsigned int addr, unsigned int value) {
  byte highByte = (value >> 8) & 0xFF; // Extract the upper 8 bits
  byte lowByte = value & 0xFF;         // Extract the lower 8 bits

  Wire.beginTransmission(EEPROM_ADDRESS);
  Wire.write((byte)(addr >> 8)); // MSB of EEPROM address
  Wire.write((byte)(addr & 0xFF)); // LSB of EEPROM address
  Wire.write(highByte); // High byte of the sensor value
  Wire.endTransmission();
  delay(5); // Write cycle time

  Wire.beginTransmission(EEPROM_ADDRESS);
  Wire.write((byte)((addr + 1) >> 8)); // MSB of the next EEPROM address
  Wire.write((byte)((addr + 1) & 0xFF)); // LSB of the next EEPROM address
  Wire.write(lowByte); // Low byte of the sensor value
  Wire.endTransmission();
  delay(5); // Write cycle time
}

unsigned int readEEPROM(unsigned int addr) {
  unsigned int highByte = 0;
  unsigned int lowByte = 0;

  Wire.beginTransmission(EEPROM_ADDRESS);
  Wire.write((byte)(addr >> 8)); // MSB
  Wire.write((byte)(addr & 0xFF)); // LSB
  Wire.endTransmission();
  delay(5); // Delay for EEPROM to process

  Wire.requestFrom(EEPROM_ADDRESS, 1); // Request 1 byte
  if (Wire.available()) {
    highByte = Wire.read(); // Read high byte
  }

  Wire.beginTransmission(EEPROM_ADDRESS);
  Wire.write((byte)((addr + 1) >> 8)); // MSB
  Wire.write((byte)((addr + 1) & 0xFF)); // LSB
  Wire.endTransmission();
  delay(5); // Delay for EEPROM to process

  Wire.requestFrom(EEPROM_ADDRESS, 1); // Request 1 byte
  if (Wire.available()) {
    lowByte = Wire.read(); // Read low byte
  }

  return (highByte << 8) | lowByte; // Combine bytes
}

void sendEEPROMData() {
  unsigned int readAddress = 3;
  unsigned int lastWrittenAddress = readEEPROM(0);

  if (lastWrittenAddress >= 3) {
    while (readAddress <= lastWrittenAddress) {
      unsigned int sensorValue = readEEPROM(readAddress);
      Serial.print("Address ");
      Serial.print(readAddress);
      Serial.print(": ");
      Serial.println(sensorValue);
      readAddress += 2;
    }
    Serial.println("END"); // Send end marker
  } else {
    Serial.println("No valid data in EEPROM to read.");
  }
}

void processCommand(char command) {
  switch (command) {
    case '1':
      sensorStates[0] = !sensorStates[0];
      break;
    case '2':
      sensorStates[1] = !sensorStates[1];
      break;
    case '3':
      sensorStates[2] = !sensorStates[2];
      break;
    case '4':
      sensorStates[3] = !sensorStates[3];
      break;
    case 'S':
      for (int i = 0; i < 4; i++) {
        Serial.print(sensorStates[i]);
        Serial.print(" ");
      }
      Serial.println(); // Send newline for clarity
      break;
  }
  writeSensorStatesToEEPROM(); // Write sensor states to EEPROM after any change
}

void writeSensorStatesToEEPROM() {
  byte packedStates = 0;
  for (int i = 0; i < 4; i++) {
    if (sensorStates[i]) {
      packedStates |= (1 << i);
    }
  }
  Wire.beginTransmission(EEPROM_ADDRESS);
  Wire.write(0x00); // MSB of EEPROM address
  Wire.write(0x02); // LSB of EEPROM address
  Wire.write(packedStates);
  Wire.endTransmission();
  delay(5); // Write cycle time
}

void readSensorStatesFromEEPROM() {
  Wire.beginTransmission(EEPROM_ADDRESS);
  Wire.write(0x00); // MSB of EEPROM address
  Wire.write(0x02); // LSB of EEPROM address
  Wire.endTransmission();
  delay(5); // Read cycle time

  Wire.requestFrom(EEPROM_ADDRESS, 1); // Request 1 byte
  if (Wire.available()) {
    byte packedStates = Wire.read();
    for (int i = 0; i < 4; i++) {
      sensorStates[i] = (packedStates & (1 << i)) != 0;
    }
  }
}

