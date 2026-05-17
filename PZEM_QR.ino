#include "Display.h"
#include "ui_qr.h"
#include "ui_pzem.h"
#include "ui_guide.h"
#include "sepay.h"
#include "pzem_manager.h"
#include "mqtt_gateway.h"

// ===== PIN / HARDWARE =====
static constexpr uint8_t  BTN_0            = 36;
static constexpr uint8_t  BTN_1            = 35;
static constexpr uint8_t  BTN_2            = 34;
static constexpr uint8_t  RELAY            = 13;

static constexpr uint8_t  PZEM_RX          = 16;
static constexpr uint8_t  PZEM_TX          = 17;
static constexpr uint32_t PZEM_BAUD        = 9600;

static constexpr float    MAX_POWER_W      = 400.0f;
static constexpr uint32_t OVERLOAD_MS      = 10000;
static constexpr uint32_t PZEM_INTERVAL    = 500;
static constexpr uint32_t BTN_DEBOUNCE_MS  = 400;

// ===== OBJECTS =====
DisplayManager display;
SepayManager   sepay;
PzemManager    pzem;
MqttGateway    mqtt;

// ===== STATE =====
enum ScreenState : uint8_t {
    SCREEN_GUIDE,
    SCREEN_QR,
    SCREEN_PZEM,
    SCREEN_EMERGENCY
};

// Trạng thái overload được tách riêng để tránh nhầm lẫn với SCREEN_EMERGENCY do người dùng
enum OverloadState : uint8_t {
    OVERLOAD_NONE,
    OVERLOAD_WARNING,   // Đang vượt ngưỡng, chưa đủ thời gian
    OVERLOAD_TRIPPED    // Đã ngắt relay do quá tải
};

static ScreenState   currentScreen      = SCREEN_GUIDE;
static OverloadState overloadState      = OVERLOAD_NONE;
static uint8_t       overloadRetryCount = 0; // Số lần đã cho phép resume sau quá tải
static uint32_t      lastButtonPress    = 0;
static uint32_t      lastPzemUpdate     = 0;
static uint32_t      overloadStart      = 0;

// Shared state giữa các task — bảo vệ bằng mutex
static float         paidKwh         = 0.0f;
static char          customerPhone[20] = {};
static SemaphoreHandle_t xStateMutex;   // Bảo vệ paidKwh & customerPhone
static SemaphoreHandle_t xDisplayMutex; // Bảo vệ display (SPI/I2C không re-entrant)

// ===== IPC QUEUES =====
struct PaymentData {
    float kwh;
    char  content[100];
};

struct MqttRequest {
    char topic[64];
    char payload[256];
};

static QueueHandle_t xPaymentQueue;
static QueueHandle_t xMqttQueue;

static uint32_t ignoreOverloadUntil = 0;

// ===== FORWARD DECLARATIONS =====
void handleButton();
void handlePayment();
void handleCharging();
void pauseCharging(bool byUser, float currentKwh);
void resumeCharging();
void stopCharging(const char* reason);

// Wrapper an toàn cho display — luôn dùng mutex trước khi vẽ
template<typename Fn>
inline void displaySafe(Fn fn) {
    if (xSemaphoreTake(xDisplayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        fn();
        xSemaphoreGive(xDisplayMutex);
    }
}

// ============================================================
//  TASK: Network (Core 0)
// ============================================================
void taskNetwork(void* pvParam) {
    for (;;) {
        mqtt.loop();
        sepay.update();

        // Nhận thanh toán từ SePay → gửi qua Queue cho Main task
        float kwh = sepay.getPaidKwh();
        if (kwh > 0.0f) {
            PaymentData pd;
            pd.kwh = kwh;
            strncpy(pd.content, sepay.getLastContent().c_str(), sizeof(pd.content) - 1);
            pd.content[sizeof(pd.content) - 1] = '\0';

            if (xQueueSend(xPaymentQueue, &pd, pdMS_TO_TICKS(100)) == pdTRUE) {
                sepay.clearPaidKwh();
            }
        }

        // Xử lý gửi MQTT từ Queue
        MqttRequest req;
        while (xQueueReceive(xMqttQueue, &req, 0) == pdTRUE) {
            // Retry nếu publish thất bại (dữ liệu thanh toán quan trọng)
            for (int attempt = 0; attempt < 3; attempt++) {
                if (mqtt.publish(req.topic, req.payload)) break;
                vTaskDelay(pdMS_TO_TICKS(200));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================
//  TASK: Main (Core 1)
// ============================================================
void taskMain(void* pvParam) {
    for (;;) {
        handleButton();
        handlePayment();
        handleCharging();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
    Serial.begin(115200);

    Serial2.begin(PZEM_BAUD, SERIAL_8N1, PZEM_RX, PZEM_TX);
    pzem.begin(Serial2);

    pinMode(BTN_0, INPUT);
    pinMode(BTN_1, INPUT);
    pinMode(BTN_2, INPUT);
    pinMode(RELAY, OUTPUT);
    digitalWrite(RELAY, LOW);

    // Tạo mutex trước khi dùng display hoặc shared state
    xStateMutex   = xSemaphoreCreateMutex();
    xDisplayMutex = xSemaphoreCreateMutex();
    configASSERT(xStateMutex   != NULL);
    configASSERT(xDisplayMutex != NULL);

    display.begin();
    
    if (digitalRead(BTN_0) == LOW) {
        Serial.println("[BOOT] BTN_0 giữ lúc khởi động — Reset WiFi!");

        display.tft.fillScreen(TFT_BLACK);
        display.tft.setTextDatum(MC_DATUM);

        display.tft.setTextSize(2);
        display.tft.setTextColor(TFT_RED, TFT_BLACK);
        display.tft.drawString("RESET WiFi...", 160, 80);

        display.tft.setTextSize(1);
        display.tft.setTextColor(TFT_WHITE, TFT_BLACK);
        display.tft.drawString("Ket noi vao Wi-Fi: SacDien-Setup", 160, 125);
        display.tft.drawString("Mo trinh duyet -> 192.168.4.1", 160, 143);
        display.tft.drawString("Nhap ten & mat khau WiFi moi", 160, 161);

        delay(2000);
        sepay.resetWiFi();
    }

    displaySafe([&]{ display.drawWelcome(); });

    sepay.begin();
    mqtt.begin();

    xPaymentQueue = xQueueCreate(2, sizeof(PaymentData));
    xMqttQueue    = xQueueCreate(8, sizeof(MqttRequest)); // Tăng lên 8 để tránh mất bản tin

    configASSERT(xPaymentQueue != NULL);
    configASSERT(xMqttQueue    != NULL);

    // NET: priority 1, stack 10240, Core 0
    // MAIN: priority 2, stack 8192 (tăng từ 4096), Core 1
    xTaskCreatePinnedToCore(taskNetwork, "NET",  10240, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(taskMain,    "MAIN",  8192, NULL, 2, NULL, 1);
}

void loop() { vTaskDelay(portMAX_DELAY); }

// ============================================================
//  XỬ LÝ NÚT BẤM
// ============================================================
void handleButton() {
    const uint32_t now = millis();
    if (now - lastButtonPress < BTN_DEBOUNCE_MS) return;

    // Đọc tất cả button trước, xử lý sau — tránh return sớm bỏ sót nút
    const bool btn0 = (digitalRead(BTN_0) == LOW);
    const bool btn1 = (digitalRead(BTN_1) == LOW);
    const bool btn2 = (digitalRead(BTN_2) == LOW);

    if (!btn0 && !btn1 && !btn2) return; // Không có nút nào nhấn

    lastButtonPress = now;

    // BTN_0: Tạm dừng sạc (người dùng chủ động)
    if (btn0 && currentScreen == SCREEN_PZEM && overloadState == OVERLOAD_NONE) {
        float tempKwh = pzem.getEnergyKwh();
        pauseCharging(true, tempKwh);
        return;
    }

    if (btn0 && currentScreen == SCREEN_EMERGENCY && overloadState == OVERLOAD_NONE) {
        float tempKwh = pzem.getEnergyKwh();
        stopCharging("manual");
        return;
    }

    // BTN_2: Tiếp tục sạc sau khi người dùng xác nhận
    if (btn2 && currentScreen == SCREEN_EMERGENCY) {
        if (overloadState == OVERLOAD_TRIPPED) {
            if (overloadRetryCount < 1) {
                // Lần 1: cho phép thử lại
                overloadRetryCount++;
                resumeCharging();
            } else {
                // Lần 2 vẫn quá tải → dừng hẳn
                Serial.println("[WARN] Quá tải lần 2 — dừng sạc.");
                stopCharging("overload");
            }
        } else {
            // Dừng do người dùng chủ động (BTN_0) → resume bình thường
            resumeCharging();
        }
        return;
    }

    // BTN_1: Điều hướng màn hình guide ↔ QR
    if (btn1) {
        if (currentScreen == SCREEN_GUIDE) {
            currentScreen = SCREEN_QR;
            displaySafe([&]{ display.drawQR(); });
        } else if (currentScreen == SCREEN_QR) {
            currentScreen = SCREEN_GUIDE;
            displaySafe([&]{ display.drawWelcome(); });
        }
    }
}

// ============================================================
//  XỬ LÝ THANH TOÁN
// ============================================================
void handlePayment() {
    PaymentData pd;
    if (xQueueReceive(xPaymentQueue, &pd, 0) != pdTRUE) return;

    // Ghi shared state dưới mutex
    if (xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        String phone = mqtt.extractPhone(pd.content);
        strncpy(customerPhone, phone.c_str(), sizeof(customerPhone) - 1);
        customerPhone[sizeof(customerPhone) - 1] = '\0';
        paidKwh = pd.kwh;
        xSemaphoreGive(xStateMutex);
    } else {
        Serial.println("[WARN] handlePayment: không lấy được mutex state");
        return;
    }

    pzem.resetEnergy();
    
    lastPzemUpdate = millis(); 
    ignoreOverloadUntil = millis() + 3500; // TẠO THỜI GIAN CHỜ 2.5s CHO PZEM UPDATE

    overloadState      = OVERLOAD_NONE;
    overloadStart      = 0;
    overloadRetryCount = 0; 

    currentScreen = SCREEN_PZEM;
    digitalWrite(RELAY, HIGH);
    displaySafe([&]{ display.drawPZEM(); });
}

// ============================================================
//  LOGIC SẠC
// ============================================================
void handleCharging() {
    if (currentScreen != SCREEN_PZEM) return;

    const uint32_t now = millis();

    // 1. NHỊP ĐỌC PZEM (1.5 giây)
    if (now - lastPzemUpdate < 1500) return;
    lastPzemUpdate = now; 

    float currentPower = pzem.getPower();
    float usedKwh      = pzem.getEnergyKwh();
    float currentAmps  = pzem.getCurrent(); 

    if (isnan(currentPower) || isnan(usedKwh) || currentPower < 0) return; 

    if (currentAmps < 0.1f) currentPower = 0.0f;

    // --- CHỈ KIỂM TRA KHI ĐÃ HẾT THỜI GIAN CHỜ RESET (ignoreOverloadUntil) ---
    if (now > ignoreOverloadUntil) {
        
        // A. KIỂM TRA ĐỦ KWH (Đã chuyển vào đây)
        float target = 0.0f;
        if (xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            target = paidKwh;
            xSemaphoreGive(xStateMutex);
        }

        if (target > 0.0f && usedKwh >= target) {
            Serial.printf("[INFO] Hoan thanh: %.3f/%.1f kWh. Dung sac!\n", usedKwh, target);
            stopCharging("done");
            return;
        }

        if (currentPower > MAX_POWER_W) {
            if (overloadState == OVERLOAD_NONE) {
                overloadState = OVERLOAD_WARNING;
                overloadStart = now;
            } 
            else if (overloadState == OVERLOAD_WARNING) {
                if (now - overloadStart >= OVERLOAD_MS) {
                    overloadState = OVERLOAD_TRIPPED;
                    if (overloadRetryCount >= 1) stopCharging("overload");
                    else pauseCharging(false, usedKwh); 
                    return; 
                }
            }
        } 
        else {
            if (overloadState == OVERLOAD_WARNING) {
                overloadState = OVERLOAD_NONE;
                overloadStart = 0; 
                displaySafe([&]{
                    if (currentScreen == SCREEN_PZEM) {
                        display.drawPZEM(); 
                        updatePZEM(display.tft, pzem, target);
                    }
                });
            }
        }
    }

    // 4. CẬP NHẬT GIAO DIỆN BÌNH THƯỜNG
    static uint32_t lastDisplayUpdate = 0;
    if ((overloadState == OVERLOAD_NONE || overloadState == OVERLOAD_WARNING) && currentScreen == SCREEN_PZEM) {
        // Cập nhật màn hình 1.5s một lần khớp với thời gian đọc
        if (now - lastDisplayUpdate >= 1500) {
            lastDisplayUpdate = now;
            displaySafe([&]{ updatePZEM(display.tft, pzem, paidKwh); });
        }
    }
    static uint32_t lastWarningUpdate = 0;
    if (overloadState == OVERLOAD_WARNING && currentScreen == SCREEN_PZEM) {
        if (now - lastWarningUpdate >= 500) {
            lastWarningUpdate = now;
            
            // Tính số giây còn lại trước khi ngắt (OVERLOAD_MS = 10000)
            int timeLeft = (OVERLOAD_MS - (now - overloadStart)) / 1000;
            if (timeLeft < 0) timeLeft = 0;

            // Tính toán nhấp nháy: chẵn -> Trắng/Đỏ, lẻ -> Vàng/Đỏ
            bool blink = (now / 500) % 2 == 0;
            
            displaySafe([&]{
                // Vẽ banner nền đỏ ở sát mép trên màn hình (Cao 30px, Rộng 320px)
                display.tft.fillRect(0, 0, 320, 30, TFT_RED);
                display.tft.setTextSize(2);
                display.tft.setTextDatum(MC_DATUM); // Căn giữa hộp
                
                if (blink) {
                    display.tft.setTextColor(TFT_WHITE, TFT_RED);
                } else {
                    display.tft.setTextColor(TFT_YELLOW, TFT_RED);
                }
                
                char warnMsg[32];
                snprintf(warnMsg, sizeof(warnMsg), "QUA TAI! NGAT SAU %ds", timeLeft);
                display.tft.drawString(warnMsg, 160, 15);
            });
        }
    }
}
// ============================================================
//  PAUSE / RESUME
// ============================================================
void pauseCharging(bool byUser, float currentKwh) {
    digitalWrite(RELAY, LOW);
    currentScreen = SCREEN_EMERGENCY;

    if (byUser) {
        displaySafe([&]{
            display.drawEmergency("NGAT SAC KHAN CAP"); // < Dùng biến truyền vào
        });
    } else {
        displaySafe([&]{
            display.drawEmergency("NGAT DO QUA TAI!");   // < Dùng biến truyền vào
        });
    }
}

void resumeCharging() {
    overloadState  = OVERLOAD_NONE;
    overloadStart  = 0;
    
    ignoreOverloadUntil = millis() + 2500; // BỎ QUA CHECK QUÁ TẢI 2.5s ĐẦU

    lastPzemUpdate = millis() - PZEM_INTERVAL;
    currentScreen  = SCREEN_PZEM;
    digitalWrite(RELAY, HIGH);
    displaySafe([&]{
        display.drawPZEM();                       
        updatePZEM(display.tft, pzem, paidKwh);  
    });
}

// ============================================================
//  DỪNG SẠC & GỬI MQTT
// ============================================================
void stopCharging(const char* reason) {

    currentScreen = SCREEN_GUIDE; 
    digitalWrite(RELAY, LOW);

    // Đọc shared state dưới mutex
    char   phoneCopy[sizeof(customerPhone)] = {};
    float  targetKwh = 0.0f;

    if (xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        strncpy(phoneCopy, customerPhone, sizeof(phoneCopy) - 1);
        targetKwh = paidKwh;
        xSemaphoreGive(xStateMutex);
    }

    displaySafe([&]{ display.drawWelcome(); });

    const float usedKwh = pzem.getEnergyKwh();

    // Tạo bản tin MQTT
    MqttRequest req;
    req.topic[0]   = '\0';
    req.payload[0] = '\0';
    strncpy(req.topic, TOPIC_DONE, sizeof(req.topic) - 1);
    snprintf(req.payload, sizeof(req.payload),
        "{\"event\":\"stop\",\"reason\":\"%s\",\"phone\":\"%s\","
        "\"used_kwh\":%.3f,\"target_kwh\":%.1f}",
        reason, phoneCopy, usedKwh, targetKwh
    );

    // Gửi vào queue với timeout — log nếu thất bại
    if (xQueueSend(xMqttQueue, &req, pdMS_TO_TICKS(500)) != pdTRUE) {
        Serial.printf("[ERROR] stopCharging: MQTT queue đầy, mất bản tin! reason=%s phone=%s\n",
                      reason, phoneCopy);
    }

    // Reset shared state dưới mutex
    if (xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        customerPhone[0] = '\0';
        paidKwh          = 0.0f;
        xSemaphoreGive(xStateMutex);
    }

    overloadState      = OVERLOAD_NONE;
    overloadStart      = 0;
    overloadRetryCount = 0;
}
