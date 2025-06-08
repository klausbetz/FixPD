#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RotaryEncoder.h>
#include "m5pps/M5ModulePPS.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1 // Reset pin (or -1 if shared with Arduino reset)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define PIN_ENCODER_A 22
#define PIN_ENCODER_B 23
#define PIN_BUTTON 21

M5ModulePPS pps;

RotaryEncoder encoder(PIN_ENCODER_A, PIN_ENCODER_B, RotaryEncoder::LatchMode::FOUR3);
int lastPos = 0;
bool lastButtonState = HIGH;

void setup() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // call tickEncoder every time a signal has changed.
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), tickEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_B), tickEncoder, CHANGE);
  
  Serial.begin(9600);

  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C is default I2C address
    Serial.println(F("SSD1306 Display allocation failed"));
    for (;;);
  }

  while (!pps.begin(&Wire, 7, 25, MODULE_POWER_ADDR, 400000U)) {
    Serial.println("module pps connect error");
    delay(100);
  }

  pps.setOutputVoltage(5);
  pps.setOutputCurrent(1);
  pps.setPowerEnable(true);

  display.clearDisplay();
  display.setTextSize(2); // Larger text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();
  delay(1000);
}

void loop() {
  // encoder.tick();  // Call this as often as possible in the loop()
  int newPos = encoder.getPosition();

  if (newPos != lastPos) {
    Serial.print("Encoder Position: ");
    Serial.println(newPos);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.print("Encoder:");
    display.setCursor(0, 30);
    display.print(newPos);
    display.display();

    lastPos = newPos;
  }

  bool buttonState = digitalRead(PIN_BUTTON);
  if (buttonState == LOW && lastButtonState == HIGH) {
    Serial.println("Button Pressed");
    delay(100);
  }
  lastButtonState = buttonState;

  delay(10); // debounce delay
}

void tickEncoder() {
  encoder.tick();
}
