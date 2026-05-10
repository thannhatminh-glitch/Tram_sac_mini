#include "ui_qr.h"
#include "image.h"

static void drawQRBitmap(TFT_eSPI &tft, int x, int y, int w, int h) {
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            int byteIdx = (row * w + col) / 8;
            int bitIdx  = 7 - (col % 8);
            bool isBlack = (pgm_read_byte(&qrcodembank[byteIdx]) >> bitIdx) & 1;
            tft.drawPixel(x + col, y + row, isBlack ? TFT_BLACK : TFT_WHITE);
        }
    }
}

void drawQR(TFT_eSPI &tft)
{
    tft.fillScreen(TFT_BLACK);

    int mid = tft.width() / 2;

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 10);
    tft.println("BANG GIA");

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 50);  tft.println("1 kWh - 5k");
    tft.setCursor(10, 80);  tft.println("2 kWh - 10k");
    tft.setCursor(10, 110); tft.println("3 kWh - 15k");
    tft.setCursor(10, 140); tft.println("4 kWh - 20k");
    tft.setCursor(10, 170); tft.println("5 kWh - 25k");

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(mid + 20, 10);
    tft.println("SCAN QR");

    tft.fillRect(mid, 30, QRCODEMBANK_WIDTH, QRCODEMBANK_HEIGHT, TFT_WHITE);
    drawQRBitmap(tft, mid, 30, QRCODEMBANK_WIDTH, QRCODEMBANK_HEIGHT);

    tft.setTextSize(2);
    tft.setCursor(10, 190);
    tft.println("Chuyen khoan dung");
    tft.setCursor(10, 215);
    tft.println("so tien quy dinh");
}