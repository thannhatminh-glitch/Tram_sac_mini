#pragma once
#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

// ===== CẤU HÌNH BROKER =====
// Dùng broker công khai HiveMQ — không cần tài khoản
// Hoặc đổi sang broker riêng của bạn
#define MQTT_BROKER       "broker.hivemq.com"
#define MQTT_PORT         1883
#define MQTT_CLIENT_ID    "tram_sac_esp32"

// ===== TOPICS =====
// Mở MQTT Explorer → kết nối broker.hivemq.com:1883
// Subscribe "#" hoặc "tram_sac/#" để nhận tất cả
#define TOPIC_PAYMENT     "tram_sac/payment"      // Thanh toán mới
#define TOPIC_DONE        "tram_sac/done"          // Sạc xong
#define TOPIC_STATUS      "tram_sac/status"        // Trạng thái sạc định kỳ
#define TOPIC_OVERLOAD    "tram_sac/overload"      // Cảnh báo quá tải

class MqttGateway {
public:
    void   begin();
    void   loop();                               // Gọi trong taskNetwork mỗi vòng

    // Publish bản tin (JSON string)
    bool   publish(const char* topic, const char* payload, bool retained = false);

    bool   connected();

    // Tách số điện thoại từ nội dung chuyển khoản
    String extractPhone(const String& content);

private:
    WiFiClient   _wifiClient;
    PubSubClient _mqtt;

    void   reconnect();
};