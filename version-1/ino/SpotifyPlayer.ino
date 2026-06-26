#include <NimBLEDevice.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <RTClib.h>
#include <XPT2046_Touchscreen.h>
#include <NimBLEDevice.h>
#include <BleKeyboard.h>

#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST 4
#define TS_CS 5
#define TS_IRQ 17
#define TS_CLK 18
#define TS_DIN 23
#define TS_DO 19

const int rowPin = 32;
const int colPins[3] = {33, 25, 26}; 

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TS_CS, TS_IRQ);
RTC_DS3231 rtc;
BleKeyboard bleKeyboard("Music Player", "Perlica", 100);

//Swipe
int touchStartX = 0;
int touchStartY = 0;3
int lastTouchY = 0;
unsigned long touchStartTime = 0;
const int swipeThreshold = 25; // Jarak min pixel untuk dianggap swipe
const int tapThreshold = 250;  // Waktu max (ms) untuk dianggap tap

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  
  tft.begin();
  tft.setRotation(90);
  tft.fillScreen(ILI9341_BLACK);

  ts.begin();
  ts.setRotation(0);

  if (!rtc.begin()) Serial.println("NO RTC??");

  bleKeyboard.begin();

  pinMode(rowPin, OUTPUT);
  digitalWrite(rowPin, LOW);

  for(int i=0; i<3; i++) {
    pinMode(colPins[i], INPUT_PULLDOWN); 
  }

  drawUI();
}

void loop() {
  updateTime();
  checkButtons();
  
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    delay(200); 
  }

  updateTime();
  checkButtons();
  handleTouch();
}

void handleTouch() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    lastTouchY = p.y;
    
    if (touchStartTime == 0) {
      touchStartX = p.x;
      touchStartY = p.y;
      touchStartTime = millis();
    }
  } else if (touchStartTime > 0) {
    unsigned long duration = millis() - touchStartTime;
    int deltaY = lastTouchY - touchStartY; 
    
    if (bleKeyboard.isConnected()) {
      if (deltaY < -swipeThreshold) {
        bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
        Serial.println("Prev");
      } 
      else if (deltaY > swipeThreshold) {
        bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);
        Serial.println("Next");
      } 
      else if (duration <= tapThreshold) {
        bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
        Serial.println("damn MOESICCCC");
      }
    }
    touchStartTime = 0;
    lastTouchY = 0;
  }
}

void drawUI() {
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(60, 20);
  tft.print("TIME DISPLAY");
  
  tft.drawFastHLine(0, 50, 240, ILI9341_BLUE);
  tft.drawFastHLine(0, 280, 240, ILI9341_BLUE);
}

void updateTime() {
  DateTime now = rtc.now();
  static int lastMin = -1;
  if (now.minute() != lastMin) {
    tft.setTextSize(10);
    
    // Jam
    tft.setCursor(60, 80);
    if(now.hour() < 10) tft.print('0');
    tft.print(now.hour());

    // Menit
    tft.setCursor(60, 180);
    if(now.minute() < 10) tft.print('0');
    tft.print(now.minute());
    
    lastMin = now.minute();
  }
}

void checkButtons() {
  if (!bleKeyboard.isConnected()) return;

  digitalWrite(rowPin, HIGH);
  delayMicroseconds(50);

  for (int i = 0; i < 3; i++) {
    if (digitalRead(colPins[i]) == HIGH) { 
      if (i == 0) bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
      if (i == 1) bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
      if (i == 2) bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);
      
      Serial.print("Key Pressed : "); Serial.println(i);
      
      while(digitalRead(colPins[i]) == HIGH) {
        delay(10); 
      }
    }
  }
  digitalWrite(rowPin, LOW);
}