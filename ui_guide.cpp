#include "ui_guide.h"

void drawGuide(TFT_eSPI &tft) {

    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_BLACK);

    // Tiêu đề
    tft.setTextSize(3);
    tft.setCursor(20, 10);
    tft.println("WELCOME TRAM SAC");

    // đường kẻ
    tft.drawLine(0, 45, 320, 45, TFT_BLACK);

    // Nội dung
    tft.setTextSize(2);
    tft.setCursor(10, 60);
    tft.println("Huong dan su dung:");

    tft.setCursor(5, 90);
    tft.println("1.Quet ma QR de thanh toan");

    tft.setCursor(5, 120);
    tft.println("2.Cam sac vao xe");

    tft.setCursor(5, 150);
    tft.println("3.He thong cap dien");

    // đường kẻ dưới
    tft.drawLine(10, 190, 310, 190, TFT_BLACK);

    // dòng cuối
    tft.setCursor(25, 200);
    tft.println("Nhan nut 1 de tiep tuc");
}