# 🖼️ ESP32 MQTT Image Display with ST7735 TFT LCD

This project enables an **ESP32** to receive **Base64-encoded JPEG/PNG images over MQTT** and render them on a **1.8" ST7735 TFT LCD**. It also supports image caching using **SPIFFS**, so the last received image is automatically reloaded at boot.

---

## 🔧 Hardware Connections

| LCD Pin | ESP32 GPIO |
| ------- | ---------- |
| VCC     | 3.3V       |
| GND     | GND        |
| CS      | GPIO5      |
| RESET   | GPIO4      |
| DC      | GPIO2      |
| MOSI    | GPIO23     |
| SCLK    | GPIO18     |
| LED     | 3.3V       |

> You can customize the pin configuration in `lcd_display.c`.



## 🚀 Getting Started

### 1. Configure Wi-Fi

In `app_main.c`:

```c
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASS "your_wifi_password"
```

### 2. MQTT Settings

The client subscribes to the following topics:

```c
#define MQTT_BROKER "mqtt://test.mosquitto.org"
#define MQTT_TOPIC_IMAGE "your_mqtt_photo_topic"
#define MQTT_TOPIC_TEXT  "your_mqtt_string_topic"
```

### 3. Build & Flash

```bash
idf.py set-target esp32
idf.py menuconfig   # ➤ Configure partition table (enable SPIFFS)
idf.py build
idf.py -p /dev/ttyUSB0 -b 460800 flash monitor
```

> Ensure your partition table supports SPIFFS and total Flash is set to **4MB**.

---

## 📤 Sending an Image (Python Example)

```python
import base64
import paho.mqtt.publish as publish

with open("image.jpg", "rb") as f:
    b64_data = base64.b64encode(f.read()).decode()

publish.single("your_mqtt_photo_topic", b64_data, hostname="test.mosquitto.org")
```

> Recommended image size: **128x160 pixels**, and Base64 size should stay under **8000 characters**.

---

## 💾 SPIFFS Cache Behavior

* Every received image is saved to `/spiffs/tmp.img`.
* On boot, the system tries to reload and display this cached image.
* To clear flash manually:

```bash
idf.py erase_flash
```


## 📐 Partition Table Configuration

This project requires a **custom partition table** to reserve space for both the application and SPIFFS filesystem. Use the following `partitions.csv` configuration:

```
# Name,     Type, SubType, Offset,  Size
nvs,        data, nvs,     0x9000,  0x6000
phy_init,   data, phy,     0xf000,  0x1000
factory,    app,  factory, 0x10000, 1M
spiffs,     data, spiffs,          0x100000
```

> 🛠️ You can specify this in `menuconfig`
> `→ Partition Table → Custom partition table CSV`
> Then point it to your custom `partitions.csv` file.

### ⚠️ Flash Requirements

* Total flash size must be **at least 4MB**
* SPIFFS will store the most recent image as `/spiffs/tmp.img`

---


## ⚠️ Notes

* **Image format** is automatically detected as JPEG or PNG.
* ST7735 is set to **BGR mode** by default. If you see wrong colors (e.g., red = blue), adjust the memory access control byte in `lcdInit()` (e.g., try `0xC0` for RGB).
* Partial rendering is done line-by-line using `lcdDrawMultiPixels` for performance.

---

## 🔗 References

* [ESP-IDF Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
* [Mosquitto MQTT Broker](https://test.mosquitto.org/)
* [tjpgd JPEG Decoder](http://elm-chan.org/fsw/tjpgd/00index.html)
* [pngle PNG Decoder](https://github.com/kikuchan/pngle)
* [esp-idf-m5stickC](https://github.com/nopnop2002/esp-idf-m5stickC)

---

Would you like me to help format this into a `README.md` file for GitHub with badges and screenshots?
