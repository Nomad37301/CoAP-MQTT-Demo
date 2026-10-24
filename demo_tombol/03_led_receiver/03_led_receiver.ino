#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include "NetworkConfig.h"



const int LED_PIN = 18;  // LED eksternal. Gunakan -1 untuk Serial Monitor saja.
const bool LED_ACTIVE_HIGH = true;  // true: GPIO -> resistor -> LED -> GND.
const bool LED_BOOT_TEST = true;    // Dua kedipan saat reset untuk cek rangkaian.
const uint32_t FLASH_MS = 1000;

WiFiUDP udp;
Coap coap(udp);
String lastEvent;
bool ledOn = false;
uint32_t ledStartedAt = 0;

void setLed(bool on) {
  ledOn = on;
  if (LED_PIN >= 0) {
    digitalWrite(LED_PIN, (on == LED_ACTIVE_HIGH) ? HIGH : LOW);
  }
}

bool validEvent(const String &value) {
  if (value.length() < 10 || value.length() > 19 || value[8] != ':') return false;
  for (unsigned int i = 0; i < value.length(); ++i) {
    if (i == 8) continue;
    const char c = value[i];
    if (i < 8) {
      if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    } else if (c < '0' || c > '9') return false;
  }
  return true;
}

void onBell(CoapPacket &packet, IPAddress ip, int port) {
  String payload;
  COAP_RESPONSE_CODE code;
  if (packet.code == COAP_GET) {
    payload = ledOn ? "1" : "0";
    code = COAP_CONTENT;
  } else if (packet.code == COAP_PUT) {
    for (size_t i = 0; i < packet.payloadlen; ++i) payload += (char)packet.payload[i];
    if (!validEvent(payload)) {
      payload = "Format: 8digithex:hitungan";
      code = COAP_BAD_REQUEST;
    } else {
      // Pengiriman ulang kejadian terakhir diberi ACK tanpa menyalakan ulang LED.
      if (payload != lastEvent) {
        lastEvent = payload;
        setLed(true);
        ledStartedAt = millis();
        Serial.println("BEL MASUK: " + payload + " | LED ON selama 1 detik");
      }
      code = COAP_CHANGED;
    }
  } else {
    payload = "Gunakan GET atau PUT";
    code = COAP_METHOD_NOT_ALLOWED;
  }
  coap.sendResponse(ip, port, packet.messageid, payload.c_str(), payload.length(),
                    code, COAP_TEXT_PLAIN, packet.token, packet.tokenlen);
}

void setup() {
  Serial.begin(115200);
  if (LED_PIN >= 0) {
    pinMode(LED_PIN, OUTPUT);
    setLed(false);
    Serial.printf("LED eksternal: GPIO%d | aktif %s\n", LED_PIN,
                  LED_ACTIVE_HIGH ? "HIGH" : "LOW");
    if (LED_BOOT_TEST) {
      Serial.println("TES LED: dua kedipan sebelum koneksi Wi-Fi.");
      for (int i = 0; i < 2; ++i) {
        setLed(true);
        delay(250);
        setLed(false);
        delay(250);
      }
    }
  }
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  Serial.print("RECEIVER IP: ");
  Serial.println(WiFi.localIP());
  coap.server(onBell, "bell");
  if (!coap.start()) Serial.println("GAGAL membuka UDP CoAP");
  Serial.println("Siap: PUT /bell, GET /bell.");
}

void loop() {
  coap.loop();
  if (ledOn && millis() - ledStartedAt >= FLASH_MS) {
    setLed(false);
    Serial.println("LED OFF");
  }
  delay(1);
}
