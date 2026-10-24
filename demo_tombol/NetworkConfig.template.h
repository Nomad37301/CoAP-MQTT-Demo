#pragma once
#include <WiFi.h>

const char *WIFI_SSID = "GANTI_NAMA_WIFI";
const char *WIFI_PASSWORD = "GANTI_PASSWORD_WIFI";

// Digunakan oleh pengontrol S3. Ganti sesuai IP kedua server pada jaringan demo.
IPAddress BUTTON_IP(192, 168, 1, 101);
IPAddress RECEIVER_IP(192, 168, 1, 102);
