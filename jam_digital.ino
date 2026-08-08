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

// ============ KONFIGURASI ============

// WiFi
const char* ssid     = "Live Stream";
const char* password = "";  // tanpa password

// Pin NodeMCU
#define DIN_PIN  13  // D7
#define CS_PIN   15  // D8
#define CLK_PIN  14  // D5

// LED Matrix
#define MAX_DEVICES 8
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

// NTP - zona waktu WIB (GMT+7)
const long gmtOffset = 25200;   // 7 jam x 3600 detik
const int  dstOffset = 0;

// Brightness (0-15)
#define BRIGHTNESS 5

// Nama bulan Indonesia
const char* namaBulan[] = {"JAN","FEB","MAR","APR","MEI","JUN","JUL","AGU","SEP","OKT","NOV","DES"};

// ============ OBJEK ============

MD_Parola display = MD_Parola(HARDWARE_TYPE, DIN_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

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
  Serial.println("=== Jam Digital siap! ===\n");
}

// ============ LOOP ============

void loop() {
  unsigned long currentMillis = millis();

  // Cek tombol FLASH ditekan → tampilkan IP 5 detik
  if (digitalRead(BTN_PIN) == LOW) {
    showIP = true;
    ipShowTime = currentMillis;

    if (WiFi.status() == WL_CONNECTED) {
      sprintf(displayBuf, "IP:%s", WiFi.localIP().toString().c_str());
    } else {
      sprintf(displayBuf, "NO WIFI");
    }
    display.displayText(displayBuf, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    display.displayAnimate();
    Serial.printf("Tombol ditekan! IP: %s\n", WiFi.localIP().toString().c_str());
  }

  // Kembali ke tampilan normal setelah 5 detik
  if (showIP && (currentMillis - ipShowTime >= ipShowDuration)) {
    showIP = false;
    display.displayClear();
  }

  // Jika sedang tampilkan IP, skip update waktu
  if (showIP) return;

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

    // Tampilkan tanggal setiap 10 detik (detik 50-59)
    if (detik >= 50 && detik <= 59) {
      tampilkanTanggal();
    } else {
      tampilkanWaktu();
    }

    // Serial output dengan IP
    String ipStr = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "-";
    Serial.printf("Waktu: %02d:%02d:%02d | WiFi: %s | IP: %s\n",
                  jam, menit, detik,
                  WiFi.status() == WL_CONNECTED ? "OK" : "OFF",
                  ipStr.c_str());
  }
}

// ============ TAMPILKAN JAM ============

void tampilkanWaktu() {
  if (detik % 2 == 0) {
    sprintf(displayBuf, "%02d.%02d", jam, menit);
  } else {
    sprintf(displayBuf, "%02d %02d", jam, menit);
  }
  display.displayText(displayBuf, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
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
  display.displayText(displayBuf, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  display.displayAnimate();
}
