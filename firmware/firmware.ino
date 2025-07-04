#include <Wire.h>
#include <U8g2lib.h>
#include <RotaryEncoder.h>
#include "M5ModulePPS.h"
#include <OneButton.h>

#define PIN_ENCODER_A 22
#define PIN_ENCODER_B 23
#define PIN_BUTTON 21

#define VOLTAGE_INCREMENT 100
#define CURRENT_INCREMENT 50
#define MAX_VOLTAGE 20000
#define MAX_CURRENT 5000

// 16x16
static const unsigned char ligthning_on[] U8X8_PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x03,0x80,0x01,0xc0,0x01,0xe0,0x00,0xf0,0x07,0x80,0x03,0xc0,0x01,0xc0,0x00,0x60,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

// 9x10
static const unsigned char lightning_off[] U8X8_PROGMEM = {0x40,0x01,0xa0,0x00,0x50,0x00,0xe8,0x01,0x84,0x00,0x42,0x00,0x2f,0x00,0x14,0x00,0x0a,0x00,0x05,0x00};

M5ModulePPS pps;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
RotaryEncoder encoder(PIN_ENCODER_A, PIN_ENCODER_B, RotaryEncoder::LatchMode::FOUR3);
OneButton btn = OneButton(PIN_BUTTON, true, true);

float lastReadbackVoltage = 0;
float lastReadbackCurrent = 0;
float lastInputVoltage = 0;
int lastTemparature = 0;
int lastPos = 0;

int outputVoltage = 0; // mV
int outputCurrent = 0; // mA
uint8_t outputMode = 0; // 0=off, 1=constant voltage, 2=constant current
bool needsUpdate = false;
byte cursorPos = 0; // 0=voltage, 1=current, 2=on/off

unsigned long previousMillis = 0;
const long blinkInterval = 500; // in ms
bool showLine = true;
bool mainMenuActive = true;
bool lineVisible = true;

void setup() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), tickEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_B), tickEncoder, CHANGE);
  
  btn.attachPress(handlePress);

  Serial.begin(9600);

  u8g2.begin();
  drawSplashScreen();

  while (!pps.begin(&Wire, 7, 25, 0x35, 100000U)) {
    Serial.println("PPS connection error");
    delay(1000);
  }

  pps.setOutputCurrent(0);
  pps.setOutputVoltage(0);
  pps.setPowerEnable(false);

  delay(2000);
  needsUpdate = true;
}

void loop() {
  btn.tick();
  int newPos = encoder.getPosition();
  float readbackVoltage = (int)(pps.getReadbackVoltage()*1000)/1000.0;
  float readbackCurrent = (int)(pps.getReadbackCurrent()*1000)/1000.0;
  float inputVoltage = (int)(pps.getVIN()*100)/100.0;
  int temparature = (int)pps.getTemperature();
  uint8_t mode = pps.getMode();

  if (mainMenuActive && !lineVisible) {
    lineVisible = true;
    needsUpdate = true;
  } else if (!mainMenuActive && cursorPos != 2) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      lineVisible = !lineVisible;
      needsUpdate = true;
    }
  }

  if (newPos != lastPos || readbackVoltage != lastReadbackVoltage || readbackCurrent != lastReadbackCurrent || inputVoltage != lastInputVoltage || temparature != lastTemparature || mode != outputMode) {
    if (newPos > lastPos) {
      if(mainMenuActive) {
        cursorPos = (cursorPos == 2) ? 0 : cursorPos+1;
      } else if (cursorPos == 0) {
        outputVoltage = (outputVoltage+VOLTAGE_INCREMENT >= MAX_VOLTAGE) ? MAX_VOLTAGE : outputVoltage + VOLTAGE_INCREMENT;
      } else if (cursorPos == 1) {
        outputCurrent = (outputCurrent+CURRENT_INCREMENT >= MAX_CURRENT) ? MAX_CURRENT : outputCurrent + CURRENT_INCREMENT;
      }
    } else if (newPos < lastPos) {
      if(mainMenuActive) {
        cursorPos = (cursorPos == 0) ? 2 : cursorPos-1;
      } else if (cursorPos == 0) {
        outputVoltage = (outputVoltage-VOLTAGE_INCREMENT <= 0) ? 0 : outputVoltage - VOLTAGE_INCREMENT;
      } else if (cursorPos == 1) {
        outputCurrent = (outputCurrent-CURRENT_INCREMENT <= 0) ? 0 : outputCurrent - CURRENT_INCREMENT;
      }
    }

    lastPos = newPos;
    lastReadbackVoltage = readbackVoltage;
    lastReadbackCurrent = readbackCurrent;
    lastInputVoltage = inputVoltage;
    lastTemparature = temparature;
    outputMode = mode;

    debugPrint();

    needsUpdate = true;
  }

  if (needsUpdate) {
    drawScreen();
    needsUpdate = false;
  }

  delay(20);
}

void tickEncoder() {
  encoder.tick();
}

void handlePress() {
  if(cursorPos == 2) {
    if(outputMode == 0) {
      pps.setPowerEnable(true);
    } else {
      pps.setPowerEnable(false);
    }
  } else {
    if (mainMenuActive) {
      mainMenuActive = false;
    } else {
      float v = outputVoltage / 1000.0;
      float a = outputCurrent / 1000.0;
      pps.setOutputVoltage(v);
      pps.setOutputCurrent(a);
      mainMenuActive = true;
    }
  }
}

void drawSplashScreen() {
  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  u8g2.setFont(u8g2_font_profont22_tr);
  u8g2.drawStr(34, 38, "FixPD");
  u8g2.sendBuffer();
}

void drawScreen() {
  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);

  if (lastReadbackVoltage < 10 && lastReadbackVoltage >= 0) {
    u8g2.setCursor(12, 17);
  } else {
    u8g2.setCursor(0, 17);
  }
  u8g2.setFont(u8g2_font_profont22_tr);
  u8g2.print(lastReadbackVoltage, 3);
  u8g2.print("V");

  u8g2.setFont(u8g2_font_profont12_tr);
  
  char buffer[10];
  sprintf(buffer, "%5d mV", outputVoltage);
  u8g2.drawStr(34, 28, buffer);

  if(cursorPos == 0 && lineVisible) {
    u8g2.drawLine(64-(countDigits(outputVoltage)*6), 29, 80, 29);
  }

  if (lastReadbackCurrent < 10 && lastReadbackCurrent >= 0) {
    u8g2.setCursor(12, 49);
  } else {
    u8g2.setCursor(0, 49);
  }
  u8g2.setFont(u8g2_font_profont22_tr);
  u8g2.print(lastReadbackCurrent, 3);
  u8g2.print("A");

  u8g2.setFont(u8g2_font_profont12_tr);
  sprintf(buffer, "%5d mA", outputCurrent);
  u8g2.drawStr(34, 60, buffer);

  if(cursorPos == 1 && lineVisible) {
    u8g2.drawLine(64-(countDigits(outputCurrent)*6), 61, 80, 61);
  }

  if (outputMode > 0) {
    u8g2.drawXBMP(112, 3, 16, 16, ligthning_on);  
  } else {
    u8g2.drawXBMP(115, 5, 9, 10, lightning_off);
  }
  
  if (cursorPos == 2) {
    u8g2.drawRFrame(111, 2, 17, 17, 5);
  }

  u8g2.setFont(u8g2_font_profont10_tf);
  if(outputMode == 1) {
    u8g2.drawStr(118, 33, "CV");
  } else if (outputMode == 2) {
    u8g2.drawStr(118, 33, "CC");
  } else {
    u8g2.drawStr(113, 33, "OFF");
  }
  
  u8g2.setCursor(108, 42);
  u8g2.print("33");
  u8g2.write(176);
  u8g2.print("C");

  if(lastInputVoltage < 10) {
    u8g2.setCursor(103, 51);
  } else {
    u8g2.setCursor(98, 51);
  }
  u8g2.print(lastInputVoltage, 2);
  u8g2.print("V");

  float power = fabs((int)(lastReadbackVoltage * lastReadbackCurrent * 100) / 100.0);
  
  if(power < 10) {
    u8g2.setCursor(103, 60);
  } else if (power < 100) {
    u8g2.setCursor(98, 60);
  } else {
    u8g2.setCursor(95, 60);
  }

  u8g2.print(power, 2);
  u8g2.print("W");

  u8g2.sendBuffer();
}

void debugPrint() {
  Serial.print("debug: ");
  Serial.print(lastInputVoltage, 2);
  Serial.print("V, ");
  Serial.print(lastTemparature, 1);
  Serial.print("°C, ");
  Serial.print(lastReadbackVoltage, 3);
  Serial.print("V, ");
  Serial.print(lastReadbackCurrent, 3);
  Serial.print("A, ");
  Serial.print(outputVoltage/1000.0, 3);
  Serial.print("V, ");
  Serial.print(pps.getOutputVoltage(), 3);
  Serial.print("V, ");
  Serial.println(lastPos);
}

char countDigits(int number) {
  char currentNumberOfDigits = 0;
  do {
    number = number / 10;
    currentNumberOfDigits++;
  } while (number != 0);

  return currentNumberOfDigits;
}
