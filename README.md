# 🕐 Jam Digital WiFi NTP - LED Matrix

Jam digital berbasis **NodeMCU ESP8266** dengan **8x MAX7219 LED Matrix 8x8** (daisy chain), menampilkan waktu dari server NTP, tanggal, dan IP address.

---

## ✨ Fitur

- **Jam digital** format `HH.MM` dengan kedip titik dua setiap detik
- **Tanggal** otomatis (`DD MMM YYYY`) tampil setiap 10 detik
- **WiFi + NTP** — waktu akurat dari internet (GMT+7 WIB)
- **Scroll IP address** via tombol FLASH (tahan 5 detik)
- **Pesan selamat datang** saat boot dengan scrolling text
- **Kata motivasi random** tampil setelah welcome screen
- **DHT11 sensor** (opsional) untuk suhu & kelembaban _(disabled by default)_
- **Serial Monitor** — info waktu, WiFi, IP, dan sensor setiap detik

---

## 🔧 Komponen

| Komponen               | Jumlah | Keterangan                    |
| ---------------------- | ------ | ----------------------------- |
| NodeMCU ESP8266        | 1      | Board utama                   |
| MAX7219 LED Matrix 8x8 | 8      | Dua set 4 module, daisy chain |
| DHT11 Sensor           | 1      | Opsional (suhu & kelembaban)  |
| Kabel jumper           | -      | Sesuai kebutuhan              |

---

## ⚡ Wiring

### LED Matrix (MAX7219)

| MAX7219 | NodeMCU | GPIO   |
| ------- | ------- | ------ |
| DIN     | D5      | GPIO14 |
| CS      | D6      | GPIO12 |
| CLK     | D7      | GPIO13 |
| VCC     | 5V      | —      |
| GND     | GND     | —      |

### DHT11 Sensor _(opsional)_

| DHT11 | NodeMCU | GPIO  |
| ----- | ------- | ----- |
| VCC   | 3.3V    | —     |
| DATA  | D4      | GPIO2 |
| GND   | GND     | —     |

### Tombol FLASH

| Tombol           | NodeMCU | GPIO  |
| ---------------- | ------- | ----- |
| FLASH (built-in) | D3      | GPIO0 |

> Tombol FLASH bawaan NodeMCU, tidak perlu wiring tambahan.

---

## 📦 Library yang Dibutuhkan

Install via **Arduino IDE → Tools → Manage Libraries**:

1. **MD_Parola** (by majicdesigns)
2. **MD_MAX72xx** (by majicdesigns)
3. **DHT sensor library** (by Adafruit) — _opsional, hanya jika DHT enabled_

> Library `ESP8266WiFi`, `SPI`, dan `time.h` sudah built-in di board ESP8266.

---

## 🛠️ Setup Arduino IDE

1. Tambah board ESP8266 di **File → Preferences → Additional Board URLs**:
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
2. Install **esp8266** di Board Manager
3. Pilih board: **NodeMCU 1.0 (ESP-12E Module)**
4. Upload speed: **115200**

---

## ⚙️ Konfigurasi

Ubah konfigurasi di bagian atas file `jam_digital.ino`:

```cpp
// WiFi
const char* ssid     = "Live Stream";   // Nama WiFi
const char* password = "";              // Password WiFi

// NTP - zona waktu WIB (GMT+7)
const long gmtOffset = 25200;           // 7 jam x 3600 detik

// Brightness (0-15)
#define BRIGHTNESS 2

// DHT11 (opsional)
#define DHT_ENABLED false               // Ubah ke true untuk aktifkan
```

---

## 📺 Tampilan LED Matrix

| Detik   | Tampilan                               |
| ------- | -------------------------------------- |
| 0 - 39  | Jam (`HH.MM`) dengan kedip             |
| 40 - 49 | Tanggal (`8 AGU 2026`)                 |
| 50 - 59 | Suhu & Kelembaban _(jika DHT enabled)_ |

> Tekan **tombol FLASH** kapan saja untuk melihat IP address (scrolling, 5 detik).

---

## 📂 Struktur File

```
jam_digital/
├── jam_digital.ino    # Kode utama
└── README.md          # Dokumentasi ini
```

---

## 📝 Catatan

- Pastikan **urutan daisy chain** MAX7219 benar (DOUT module 1 → DIN module 2, dst.)
- Kecerahan default `BRIGHTNESS = 2` (redup), naikkan jika terlalu gelap
- WiFi tanpa password: kosongkan `password = ""`
- Untuk mengaktifkan DHT11, set `DHT_ENABLED = true` dan pastikan library DHT ter-install
- Pin D6 (GPIO12) sensitif saat boot — pastikan MAX7219 tidak menarik pin HIGH saat power-on

---

## 📜 License

Proyek pribadi / edukasi. Bebas digunakan dan dimodifikasi.
