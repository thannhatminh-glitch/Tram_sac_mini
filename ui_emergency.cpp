#include "ui_emergency.h"

// ─── Màu nền lưới (hex 16-bit RGB565) ────────────────────────────────────────
#define COLOR_DARK_RED   0x6000   // #C00000
#define COLOR_STRIPE     0x3800   // #700000 – sọc xen kẽ để tạo cảm giác nguy hiểm

// ─── Vẽ nền sọc đen/đỏ đậm (pattern cảnh báo) ───────────────────────────────
static void drawHazardStripes(TFT_eSPI &tft) {
    for (int y = 0; y < 240; y += 12) {
        tft.fillRect(0, y,     320, 6,  COLOR_DARK_RED);
        tft.fillRect(0, y + 6, 320, 6,  TFT_BLACK);
    }
}

// ─── Biểu tượng tia sét (cảnh báo) ở giữa màn hình ──────────────────────────
// Vẽ đơn giản bằng các đường thẳng, không cần bitmap
static void drawBoltIcon(TFT_eSPI &tft, int cx, int cy) {
    // Hình tia sét: 2 đường thẳng tạo hình "Z"
    uint16_t c = TFT_YELLOW;
    // Đỉnh trên cùng → giữa
    tft.drawLine(cx + 6,  cy,      cx - 2,  cy + 14, c);
    tft.drawLine(cx + 7,  cy,      cx - 1,  cy + 14, c);
    tft.drawLine(cx + 8,  cy,      cx,      cy + 14, c);
    // Giữa → đáy
    tft.drawLine(cx - 2,  cy + 14, cx + 10, cy + 14, c);
    tft.drawLine(cx + 10, cy + 14, cx - 4,  cy + 28, c);
    tft.drawLine(cx + 11, cy + 14, cx - 3,  cy + 28, c);
    tft.drawLine(cx + 12, cy + 14, cx - 2,  cy + 28, c);
}

// ─── Giao diện chính ─────────────────────────────────────────────────────────
void drawEmergencyUI(TFT_eSPI &tft, const char* reason) {

    // --- Nền ---
    drawHazardStripes(tft);

    // --- Hộp trung tâm (nền đen mờ, bo góc giả bằng fillRect) ---
    // Hộp 300×180, bắt đầu từ (10, 30)
    const int16_t BX = 10, BY = 28, BW = 300, BH = 184;
    tft.fillRect(BX,     BY,     BW,     BH,     TFT_BLACK);
    tft.drawRect(BX,     BY,     BW,     BH,     TFT_RED);
    tft.drawRect(BX + 1, BY + 1, BW - 2, BH - 2, TFT_RED); // viền đôi

    // --- Nhãn "NGAT SAC" nền đỏ ở đỉnh hộp ---
    tft.fillRect(BX + 2, BY + 2, BW - 4, 26, TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("!! NGAT SAC KHAN CAP !!", BX + BW / 2, BY + 15);

    // --- Icon tia sét (x=148~160, y=60~90) ---
    drawBoltIcon(tft, 152, 62);

    // --- Lý do ngắt ---
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    const char* msg = (reason && reason[0] != '\0') ? reason : "QUA TAI / LOI HE THONG";
    tft.drawString(msg, BX + BW / 2, BY + 108);

    // --- Đường kẻ phân cách ---
    tft.drawLine(BX + 10, BY + 122, BX + BW - 10, BY + 122, TFT_DARKGREY);

    // --- Hướng dẫn bấm nút (vùng dưới cùng màn hình) ---
    tft.fillRect(0, 220, 320, 20, TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Nhan nut 2 de tiep tuc sac", 160, 229);
}