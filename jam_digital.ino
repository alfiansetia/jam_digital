// Jam Digital - 8x MAX7219 LED Matrix 8x8 (Daisy Chain)
// Board: NodeMCU ESP8266
//
// Fitur: WiFi + NTP + Jam + Tanggal
// Tampilan: HH.MM (kedip) / DD MMM YYYY (setiap 10 detik)
// Serial: IP, waktu, status WiFi
//
// Wiring: DIN->D7  CS->D8  CLK->D5  VCC->5V  GND->GND
// Library: MD_Parola, MD_MAX72xx

// Include ESP8266WiFi DULU, lalu hapus guard-nya supaya MD_Parola tidak error
#include <ESP8266WiFi.h>
#undef _WIFI_H_   // sembunyikan dari MD_Parola

#define MD_PAROLA_USE_LIBRARY_WIFI 0
#define MD_MAX72XX_USE_LIBRARY_SPI 1

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

#include <time.h>
#if DHT_ENABLED
#include <DHT.h>
#endif

// ============ KONFIGURASI ============

// DHT11 Sensor (set ke false untuk disable)
#define DHT_ENABLED false
#define DHT_PIN   4   // D4 (GPIO2)
#define DHT_TYPE  DHT11

// WiFi
const char* ssid     = "Live Stream";
const char* password = "";  // tanpa password

// Pin NodeMCU (D5/D6/D7)
#define DIN_PIN  14  // D5 (GPIO14)
#define CS_PIN   12  // D6 (GPIO12)
#define CLK_PIN  13  // D7 (GPIO13)

// LED Matrix
#define MAX_DEVICES 8
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

// NTP - zona waktu WIB (GMT+7)
const long gmtOffset = 25200;   // 7 jam x 3600 detik
const int  dstOffset = 0;

// Brightness (0-15)
#define BRIGHTNESS 2

// Welcome & Motivasi (detik)
#define WELCOME_DUR  3   // durasi tampilan selamat datang
#define MOTIVASI_DUR 4   // durasi tampilan kata motivasi

// Kata motivasi (Indonesia)
const char* motivasi[] = {
  "Semangat pagi!",
  "Tetap belajar, tetap hebat!",
  "Hari ini lebih baik dari kemarin",
  "Usaha kecil, hasil besar",
  "Jangan menyerah!",
  "Kamu bisa, pasti bisa!",
  "Fokus pada tujuanmu",
  "Waktu adalah emas",
  "Belajar dari kemarin, hidup untuk hari ini",
  "Teruslah mencoba!"
};
const int JUMLAH_MOTIVASI = sizeof(motivasi) / sizeof(motivasi[0]);

// Nama bulan Indonesia
const char* namaBulan[] = {"JAN","FEB","MAR","APR","MEI","JUN","JUL","AGU","SEP","OKT","NOV","DES"};

// ============ OBJEK ============

MD_Parola display = MD_Parola(HARDWARE_TYPE, DIN_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
#if DHT_ENABLED
DHT dht(DHT_PIN, DHT_TYPE);
#endif

// Forward declarations
void tampilkanWaktu();
void tampilkanTanggal();
#if DHT_ENABLED
void tampilkanSensor();
#endif

// ============ VARIABEL ============

unsigned long prevMillis = 0;
const long interval = 1000;

int jam, menit, detik;
char displayBuf[20];

// Tombol FLASH (GPIO0 / D3)
#define BTN_PIN 0
bool showIP = false;
unsigned long ipShowTime = 0;
const long ipShowDuration = 5000;  // 5 detik

// ============ SETUP ============

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== Jam Digital WiFi NTP ===");

  // Tombol FLASH sebagai input
  pinMode(BTN_PIN, INPUT_PULLUP);

#if DHT_ENABLED
  // Inisialisasi DHT11
  dht.begin();
#endif

  // Inisialisasi display
  display.begin();
  display.setIntensity(BRIGHTNESS);
  display.displayClear();

  // Tampilkan "WiFi.." saat menghubungkan
  display.displayText("WiFi..", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  display.displayAnimate();
  Serial.println("Menghubungkan WiFi...");

  // Koneksi WiFi
  WiFi.begin(ssid, password);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 40) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi terhubung!");
    Serial.print("IP Address : ");
    Serial.println(WiFi.localIP());

    display.displayText("OK", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    display.displayAnimate();
    delay(1500);

    // NTP dengan configTime bawaan ESP8266
    Serial.println("Mengambil waktu NTP...");
    display.displayText("NTP..", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    display.displayAnimate();

    configTime(gmtOffset, dstOffset, "pool.ntp.org", "time.nist.gov");

    struct tm timeinfo;
    int ntpRetry = 0;
    while (!getLocalTime(&timeinfo) && ntpRetry < 20) {
      delay(500);
      Serial.print("*");
      ntpRetry++;
    }

    if (getLocalTime(&timeinfo)) {
      jam   = timeinfo.tm_hour;
      menit = timeinfo.tm_min;
      detik = timeinfo.tm_sec;
      Serial.printf("NTP OK: %02d:%02d:%02d (WIB)\n", jam, menit, detik);
    } else {
      Serial.println("NTP gagal, gunakan waktu manual");
      jam = 12; menit = 0; detik = 0;
    }
  } else {
    Serial.println("\nWiFi gagal! Waktu manual.");
    jam = 12; menit = 0; detik = 0;
  }

  display.displayClear();

  // === SELAMAT DATANG ===
  Serial.println("Menampilkan pesan selamat datang...");
  display.displayText("SELAMAT DATANG", PA_CENTER, 70, WELCOME_DUR * 1000, PA_SCROLL_LEFT, PA_MESH);
  while (!display.displayAnimate()) { yield(); }
  delay(300);

  // === MOTIVASI RANDOM ===
  randomSeed(millis());
  int idx = random(JUMLAH_MOTIVASI);
  Serial.printf("Motivasi: %s\n", motivasi[idx]);

  char motivBuf[80];
  sprintf(motivBuf, "  %s  ", motivasi[idx]);
  display.displayText((const char*)motivBuf, PA_LEFT, 50, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  unsigned long motivStart = millis();
  while (!display.displayAnimate()) {
    yield();
    if (millis() - motivStart > MOTIVASI_DUR * 1000) break;
  }

  display.displayClear();
  Serial.println("=== Jam Digital siap! ===\n");
}

// ============ LOOP ============

void loop() {
  unsigned long currentMillis = millis();

  // Cek tombol FLASH ditekan → scroll IP 5 detik
  if (digitalRead(BTN_PIN) == LOW && !showIP) {
    showIP = true;
    ipShowTime = currentMillis;

    if (WiFi.status() == WL_CONNECTED) {
      String ipStr = WiFi.localIP().toString();
      sprintf(displayBuf, "IP: %s  ", ipStr.c_str());
    } else {
      sprintf(displayBuf, "NO WIFI  ");
    }

    // Scroll dari kanan ke kiri
    display.displayText((const char*)displayBuf, PA_LEFT, 50, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    Serial.println("Tombol ditekan! Scroll IP...");
  }

  // Jalankan animasi scroll saat showIP aktif
  if (showIP) {
    display.displayAnimate();

    // Kembali ke tampilan normal setelah 5 detik
    if (currentMillis - ipShowTime >= ipShowDuration) {
      showIP = false;
      display.displayClear();
    }
    return;
  }

  if (currentMillis - prevMillis >= interval) {
    prevMillis = currentMillis;

    // Update waktu dari NTP
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      jam   = timeinfo.tm_hour;
      menit = timeinfo.tm_min;
      detik = timeinfo.tm_sec;
    } else {
      // Hitung manual jika NTP gagal
      detik++;
      if (detik >= 60) { detik = 0; menit++; }
      if (menit >= 60) { menit = 0; jam++; }
      if (jam >= 24)   { jam = 0; }
    }

    // Tampilkan: jam (0-39), tanggal (40-49), sensor (50-59)
#if DHT_ENABLED
    if (detik >= 50 && detik <= 59) {
      tampilkanSensor();
    } else
#endif
    if (detik >= 40 && detik <= 49) {
      tampilkanTanggal();
    } else {
      tampilkanWaktu();
    }

    // Serial output dengan IP
    String ipStr = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "-";
#if DHT_ENABLED
    float suhu = dht.readTemperature();
    float humi = dht.readHumidity();
    Serial.printf("Waktu: %02d:%02d:%02d | WiFi: %s | IP: %s | %.1fC %d%%\n",
                  jam, menit, detik,
                  WiFi.status() == WL_CONNECTED ? "OK" : "OFF",
                  ipStr.c_str(),
                  isnan(suhu) ? 0.0 : suhu,
                  isnan(humi) ? 0 : (int)humi);
#else
    Serial.printf("Waktu: %02d:%02d:%02d | WiFi: %s | IP: %s\n",
                  jam, menit, detik,
                  WiFi.status() == WL_CONNECTED ? "OK" : "OFF",
                  ipStr.c_str());
#endif
  }
}

// ============ TAMPILKAN JAM ============

void tampilkanWaktu() {
  if (detik % 2 == 0) {
    sprintf(displayBuf, "%02d.%02d", jam, menit);
  } else {
    sprintf(displayBuf, "%02d %02d", jam, menit);
  }
  display.displayText((const char*)displayBuf, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  display.displayAnimate();
}

// ============ TAMPILKAN TANGGAL ============

void tampilkanTanggal() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    sprintf(displayBuf, "%d %s %d", timeinfo.tm_mday, namaBulan[timeinfo.tm_mon], timeinfo.tm_year + 1900);
  } else {
    sprintf(displayBuf, "-- --- ----");
  }
  display.displayText((const char*)displayBuf, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  display.displayAnimate();
}

// ============ TAMPILKAN SUHU & KELEMBABAN ============

#if DHT_ENABLED
void tampilkanSensor() {
  float suhu = dht.readTemperature();
  float humi = dht.readHumidity();

  Serial.printf("DHT RAW -> suhu: %.2f  humi: %.2f  isnan_suhu: %d  isnan_humi: %d\n",
                suhu, humi, isnan(suhu), isnan(humi));

  if (isnan(suhu) || isnan(humi) || (suhu == 0 && humi == 0)) {
    sprintf(displayBuf, "SENSOR ERR");
  } else {
    sprintf(displayBuf, "%.1fC %d%%", suhu, (int)humi);
  }
  display.displayText((const char*)displayBuf, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  display.displayAnimate();
}
#endif
