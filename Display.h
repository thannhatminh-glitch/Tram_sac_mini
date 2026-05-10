#ifndef DISPLAY_H
#define DISPLAY_H

#include <TFT_eSPI.h>

class DisplayManager {
public:

  TFT_eSPI tft = TFT_eSPI();

  void begin();
  void lcdPowerOn();
  void lcdPowerOff();
  void drawWelcome();
  void drawQR();
  void drawPZEM();
  void drawEmergency(const char* reason);
};

#endif