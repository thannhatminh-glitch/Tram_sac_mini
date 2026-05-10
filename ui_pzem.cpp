#include "ui_pzem.h"

static const uint16_t LINES[] = {72, 112, 152, 192};

// ── Progress bar (192–240, 48px) ─────────────────────────────────────────────
static void drawProgressBar(TFT_eSPI &tft, float pct,
                             double currentKwh, double targetKwh)
{
    pct = constrain(pct, 0.0f, 1.0f);
    const uint16_t fillColor = TFT_GREEN;           // màu cố định

    const int16_t BX = 5, BY = 200, BW = 310, BH = 22;

    // Khung
    tft.drawRect(BX, BY, BW, BH, TFT_WHITE);

    // Filled & empty
    int16_t filled = (int16_t)((BW - 2) * pct);
    if (filled > 0)
        tft.fillRect(BX + 1, BY + 1, filled, BH - 2, fillColor);
    if (filled < BW - 2)
        tft.fillRect(BX + 1 + filled, BY + 1, BW - 2 - filled, BH - 2, TFT_BLACK);
    // % in giữa thanh
    char bufPct[8];
    snprintf(bufPct, sizeof(bufPct), "%d%%", (int)(pct * 100));
    tft.setTextSize(2);

    // Nền chữ khớp với vùng đang đứng (filled hoặc empty)
    // textX = BX + BW/2 - 16 = 5 + 155 - 16 = 144
    // filled bắt đầu tại BX+1 = 6 → chữ trên filled khi filled > 144-6 = 138
    bool textOnFill = (filled > 138);
    tft.setTextColor(TFT_WHITE, textOnFill ? fillColor : TFT_BLACK);
    tft.setCursor(BX + BW / 2 - 16, BY + 3);
    tft.print(bufPct);

    // --- Dòng chú thích bên dưới thanh (y=226) ---
    tft.setTextSize(1);

    // Bên trái: đã dùng — màu cố định YELLOW
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    char bufUsed[20];
    snprintf(bufUsed, sizeof(bufUsed), "Used: %.2f kWh", currentKwh);
    tft.setCursor(6, 226);
    tft.print(bufUsed);

    // Bên phải: còn lại — màu cố định CYAN
    double remain = targetKwh - currentKwh;
    if (remain < 0) remain = 0;
    char bufRem[20];
    snprintf(bufRem, sizeof(bufRem), "Left: %.2f kWh", remain);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(170, 226);
    tft.print(bufRem);
}

// ─────────────────────────────────────────────────────────────────────────────

void drawPZEMUI(TFT_eSPI &tft) {
    tft.fillScreen(TFT_BLACK);

    // Header
    tft.fillRect(0, 0, 320, 32, TFT_BLUE);
    tft.setTextColor(TFT_YELLOW, TFT_BLUE);
    tft.setTextSize(2);
    // Căn giữa (MC = Middle Center)
    tft.setTextDatum(MC_DATUM);
    // In tại điểm giữa
    tft.drawString("PZEM-004T SENSOR", 160, 16);

    for (uint16_t y : LINES)
        tft.drawLine(0, y, 320, y, TFT_CYAN);
}

void updatePZEM(TFT_eSPI &tft, PzemManager &pzem, double targetKwh) {

    bool ok = pzem.update();
    tft.setTextSize(2);

    // ===== Voltage  (y=46) =====
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(10, 46);
    tft.print("Voltage:");
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(185, 46);
    if (ok) tft.printf("%.1f V  ", pzem.getVoltage());
    else    tft.print("--- V  ");

    // ===== Current  (y=86) =====
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(10, 86);
    tft.print("Current:");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(185, 86);
    if (ok) tft.printf("%.2f A  ", pzem.getCurrent());
    else    tft.print("--- A  ");

    // ===== Power    (y=126) =====
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(10, 126);
    tft.print("Power:");
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.setCursor(185, 126);
    if (ok) tft.printf("%.1f W  ", pzem.getPower());
    else    tft.print("--- W  ");

    // ===== Energy   (y=166) =====
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(10, 166);
    tft.print("Energy:");
    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
    tft.setCursor(130, 166);
    if (ok) tft.printf("%.2f/%.1f kWh ", pzem.getEnergyKwh(), targetKwh);
    else    tft.print("---.--/-- kWh");

    // ===== Progress bar (vùng 192–240) =====
    double kwh = ok ? pzem.getEnergyKwh() : 0.0;
    float  pct = (ok && targetKwh > 0.0) ? (float)(kwh / targetKwh) : 0.0f;
    drawProgressBar(tft, pct, kwh, targetKwh);
}