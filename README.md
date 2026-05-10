# ⚡ Trạm Sạc Mini ESP32

Hệ thống trạm sạc mini sử dụng **ESP32**, hỗ trợ **thanh toán QR**, đo điện năng tiêu thụ bằng **PZEM-004T**, hiển thị trên **TFT LCD**, và quản lý trạng thái thông qua **MQTT**.

## 📌 Tính năng chính

- 🔋 Đo điện áp, dòng điện, công suất, điện năng bằng **PZEM-004T**
- 💳 Thanh toán bằng **QR Code**
- 📺 Hiển thị giao diện trên màn hình **TFT**
- 🌐 Giao tiếp MQTT để đồng bộ trạng thái
- ⏱️ Theo dõi điện năng tiêu thụ theo kWh
- 🚨 Tự động ngắt sạc khi:
  - Hết điện năng đã mua
  - Có yêu cầu dừng khẩn cấp
- 📡 Kết nối Wi-Fi với ESP32

---

## 🛠️ Phần cứng sử dụng

| Thiết bị | Mô tả |
|----------|------|
| ESP32 | Vi điều khiển chính |
| PZEM-004T V3 | Đo điện áp, dòng điện, công suất |
| TFT LCD | Hiển thị giao diện |
| Relay | Điều khiển đóng/ngắt sạc |
| Nút nhấn | Điều hướng giao diện |
| QR Payment | Thanh toán nạp tiền |

---

## 📷 Giao diện hệ thống

> Thêm ảnh chụp màn hình hoặc ảnh phần cứng tại đây

```md
![Demo](images/demo.jpg)
```

---

## ⚙️ Thư viện sử dụng

Cài đặt các thư viện cần thiết trong Arduino IDE:

- WiFi
- PubSubClient
- ArduinoJson
- TFT_eSPI
- PZEM004Tv30
- SPI

---

## 📂 Cấu trúc project

```txt
PZEM_QR/
│── PZEM_QR.ino
│── Display.h
│── ui_qr.h
│── ui_pzem.h
│── ui_guide.h
│── sepay.h
│── pzem_manager.h
│── mqtt_gateway.h
```

---

