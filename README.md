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

## 🚀 Cách chạy project

### 1. Clone project

```bash
git clone https://github.com/thannhatminh-glitch/Tram_sac_mini.git
```

### 2. Mở bằng Arduino IDE

Mở file:

```txt
PZEM_QR.ino
```

### 3. Cấu hình Wi-Fi & MQTT

Sửa thông tin trong source code:

```cpp
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
```

và thông tin MQTT broker.

### 4. Nạp code vào ESP32

- Chọn board: **ESP32 Dev Module**
- Chọn đúng COM Port
- Upload chương trình

---

## 📊 Nguyên lý hoạt động

1. Người dùng quét **QR thanh toán**
2. Hệ thống nhận số tiền nạp
3. Quy đổi thành **kWh sử dụng**
4. ESP32 bật relay cho phép sạc
5. **PZEM-004T** đo điện năng tiêu thụ
6. Khi hết điện năng hoặc dừng khẩn cấp → relay ngắt

---

## 🧮 Công thức quy đổi

Ví dụ:

- **5000 VNĐ = 0.25 kWh**

Điện năng được tính theo:

```txt
kWh = amount / PRICE_PER_KWH × 0.25
```

---

## 👨‍💻 Tác giả

**Thân Nhật Minh**

GitHub:  
https://github.com/thannhatminh-glitch

---

## 📄 License

Project phục vụ mục đích học tập và nghiên cứu.
