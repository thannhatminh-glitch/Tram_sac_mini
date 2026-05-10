#ifndef UI_PZEM_H
#define UI_PZEM_H

#include <TFT_eSPI.h>
#include "pzem_manager.h"

void drawPZEMUI(TFT_eSPI &tft);
void updatePZEM(TFT_eSPI &tft, PzemManager &pzem, double targetKwh);

#endif