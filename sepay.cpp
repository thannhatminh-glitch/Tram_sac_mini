#include <FS.h>
using namespace fs;

#include <TFT_eSPI.h>
#include "sepay.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>


// ===== SEPAY API =====
static const char* API_KEY    = "API KEY";
static const char* API_URL    = "https://my.sepay.vn/userapi/transactions/list?limit=1";

// ===== CHẾ ĐỘ TEST =====
#define TEST_MODE false

#if TEST_MODE
  static constexpr float PRICE_PER_KWH = 1.0f;
#else
  static constexpr float PRICE_PER_KWH = 5000.0f;
#endif

// ===== CẤU HÌNH =====
static const char* WIFI_AP_NAME     = "SạcĐiện-Setup";
static const char* WIFI_AP_PASSWORD = "";
static constexpr uint16_t WIFI_TIMEOUT_S  = 180;
static constexpr uint32_t CHECK_INTERVAL_MS = 5000;

static Preferences preferences;

// ===== WIFI =====

void SepayManager::connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;

    Serial.println(F("Khởi động WiFiManager..."));

    WiFiManager wm;
    wm.setConfigPortalTimeout(WIFI_TIMEOUT_S);

    if (!wm.autoConnect(WIFI_AP_NAME, WIFI_AP_PASSWORD)) {
        Serial.println(F("Không kết nối được WiFi, reboot..."));
        delay(1000);
        ESP.restart();
    }

    Serial.print(F("WiFi đã kết nối: "));
    Serial.println(WiFi.localIP());
}

// ===== RESET WIFI =====

void SepayManager::resetWiFi() {
    Serial.println(F("Reset cấu hình WiFi..."));
    WiFiManager wm;
    wm.resetSettings();
    delay(500);
    ESP.restart();
}

// ===== LOAD / SAVE LAST ID =====
// Mở preferences 1 lần trong begin(), đóng trong destructor
// hoặc dùng namespace riêng + ghi thẳng khi cần

void SepayManager::loadLastID() {
    preferences.begin("sepay", /*readOnly=*/true);
    lastID = preferences.getString("lastID", "");
    preferences.end();
    Serial.print(F("Last transaction ID: "));
    Serial.println(lastID);
}

void SepayManager::saveLastID(const String& id) {
    preferences.begin("sepay", /*readOnly=*/false);
    preferences.putString("lastID", id.c_str());
    preferences.end();
}

// ===== GETTERS =====

float SepayManager::getPaidKwh()  const { return _paidKwh; }
void  SepayManager::clearPaidKwh()      { _paidKwh = 0.0f; }
String SepayManager::getLastContent() const { return _lastContent; }

// ===== BEGIN =====

void SepayManager::begin() {
    _lastCheckMs = 0;   // đảm bảo update() chạy ngay lần đầu
    connectWiFi();
    loadLastID();
}

// ===== UPDATE =====

void SepayManager::update() {
    const uint32_t now = millis();
    if (now - _lastCheckMs < CHECK_INTERVAL_MS) return;
    _lastCheckMs = now;

    // Chỉ reconnect khi thực sự mất kết nối (tránh gọi thừa)
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }

    // Tái sử dụng client/http trên stack — nhỏ gọn, tránh heap fragmentation
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    if (!http.begin(client, API_URL)) {
        Serial.println(F("HTTP begin failed"));
        return;
    }

    // Dùng char array thay vì String để tránh cấp phát heap
    char authHeader[80];
    snprintf(authHeader, sizeof(authHeader), "Bearer %s", API_KEY);
    http.addHeader("Authorization", authHeader);
    http.addHeader("Content-Type",  "application/json");

    const int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("API Error: %d\n", httpCode);
        http.end();
        return;
    }

    String payload = http.getString();
    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, payload);
    http.end();   // đóng ngay sau khi stream xong

    if (err) {
        Serial.print(F("JSON error: "));
        Serial.println(err.c_str());
        return;
    }

    JsonArray trans = doc["transactions"];
    if (!trans || trans.size() == 0) return;

    JsonObject latest     = trans[0];
    const char* currentID = latest["id"];
    const float amount    = latest["amount_in"].as<float>();
    const char* content   = latest["transaction_content"];

    if (!currentID) return;   // guard: ID không hợp lệ

    // Lần đầu: chỉ ghi nhớ ID
    if (lastID.isEmpty()) {
        lastID = currentID;
        saveLastID(lastID);
        return;
    }

    // Giao dịch mới
    if (lastID != currentID) {
        Serial.println(F("====== NEW TRANSACTION ======"));
        Serial.printf("Amount: %.0f VND\n", amount);
        Serial.println(content);

        if (amount >= PRICE_PER_KWH) {
            _paidKwh     = floorf(amount / PRICE_PER_KWH *10.0f) / 10.0f;
            //_paidKwh = 0.01f;
            _lastContent = content;
            Serial.printf("PAYMENT ACCEPTED — %.1f kWh\n", _paidKwh);
        }

        lastID = currentID;
        saveLastID(lastID);
    }
}
