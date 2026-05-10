#include "Display.h"
#include "ui_qr.h"
#include "ui_pzem.h"
#include "ui_guide.h"
#include "ui_emergency.h"

#define LCDON 21

void DisplayManager::lcdPowerOn() {
    digitalWrite(LCDON, LOW);
}

void DisplayManager::lcdPowerOff() {
    digitalWrite(LCDON, HIGH);
}
void DisplayManager::begin() {
  pinMode(LCDON, OUTPUT);
  lcdPowerOn();
  delay(100);

  tft.init();
  tft.setRotation(3);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
  
}

void DisplayManager::drawWelcome() {

  drawGuide(tft);
}

void DisplayManager::drawQR() {

  ::drawQR(tft);
}

void DisplayManager::drawPZEM() {

  drawPZEMUI(tft);
}

void DisplayManager::drawEmergency(const char* reason) {
    drawEmergencyUI(tft, reason);
}