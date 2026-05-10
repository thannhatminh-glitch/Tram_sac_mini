#include "mqtt_gateway.h"
#include <WiFi.h>

// ===================================================================
//  begin()  —  Khởi tạo, gọi sau khi WiFi đã kết nối
// ===================================================================
void MqttGateway::begin() {
    _mqtt.setClient(_wifiClient);
    _mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    _mqtt.setKeepAlive(30);
    _mqtt.setSocketTimeout(5);

    Serial.printf("[MQTT] Broker: %s:%d\n", MQTT_BROKER, MQTT_PORT);
    reconnect();
}

// ===================================================================
//  loop()  —  Gọi định kỳ trong taskNetwork để giữ kết nối
// ===================================================================
void MqttGateway::loop() {        // ← Thêm lại hàm này
    if (!_mqtt.connected()) {
        reconnect();
    }
    _mqtt.loop();
}

void MqttGateway::reconnect() {
    if (WiFi.status() != WL_CONNECTED) return;

    uint8_t attempts = 0;
    while (!_mqtt.connected() && attempts < 3) {
        Serial.printf("[MQTT] Đang kết nối... (lần %d)\n", attempts + 1);

        _wifiClient.stop();
        delay(100); // Chờ socket đóng hoàn toàn

        if (_mqtt.connect(
                MQTT_CLIENT_ID,
                nullptr, nullptr,
                TOPIC_STATUS,
                1,
                true,
                "{\"state\":\"offline\"}"
            )) {
            Serial.println(F("[MQTT] Kết nối OK"));
            _mqtt.publish(TOPIC_STATUS, "{\"state\":\"online\"}", true);
        } else {
            Serial.printf("[MQTT] Thất bại, rc=%d\n", _mqtt.state());

            vTaskDelay(pdMS_TO_TICKS(2000));
        }
        attempts++;
    }
}

// ===================================================================
//  publish()  —  Gửi bản tin JSON lên broker
// ===================================================================
bool MqttGateway::publish(const char* topic, const char* payload, bool retained) {
    if (!_mqtt.connected()) {
        reconnect();
        if (!_mqtt.connected()) {
            Serial.printf("[MQTT] Publish thất bại (mất kết nối): %s\n", topic);
            return false;
        }
    }

    bool ok = _mqtt.publish(topic, payload, retained);
    if (ok) {
        Serial.printf("[MQTT] ✓ %s → %s\n", topic, payload);
    } else {
        Serial.printf("[MQTT] ✗ Publish lỗi: %s\n", topic);
    }
    return ok;
}

// ===================================================================
//  connected()
// ===================================================================
bool MqttGateway::connected() {
    return _mqtt.connected();
}

// ===================================================================
//  extractPhone()  —  Tách SĐT 10 số từ nội dung chuyển khoản
// ===================================================================
String MqttGateway::extractPhone(const String& content) {
    for (int i = 0; i <= (int)content.length() - 10; i++) {
        if (content[i] == '0' && isDigit(content[i + 1])) {
            String candidate = content.substring(i, i + 10);
            bool valid = true;
            for (char c : candidate) {
                if (!isDigit(c)) { valid = false; break; }
            }
            if (valid) return candidate;
        }
    }
    return "";
}