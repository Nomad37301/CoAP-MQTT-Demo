#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include "NetworkConfig.h"
#include <esp_system.h>



const uint8_t BUTTON_PIN = 18;  // GPIO18, bukan nomor urut kaki board.
const uint32_t DEBOUNCE_MS = 35;

WiFiUDP udp;
Coap coap(udp);
uint32_t bootId;
uint32_t pressCount = 0;
int rawState = HIGH;
int stableState = HIGH;
uint32_t rawChangedAt = 0;

void onButton(CoapPacket &packet, IPAddress ip, int port) {
  char payload[40];
  COAP_RESPONSE_CODE code = COAP_CONTENT;
  if (packet.code == COAP_GET) {
    // Penghitung menyimpan tekanan singkat di antara dua polling.
    snprintf(payload, sizeof(payload), "%08lx:%lu",
             (unsigned long)bootId, (unsigned long)pressCount);
  } else {
    snprintf(payload, sizeof(payload), "Gunakan GET");
    code = COAP_METHOD_NOT_ALLOWED;
  }
  coap.sendResponse(ip, port, packet.messageid, payload, strlen(payload),
                    code, COAP_TEXT_PLAIN, packet.token, packet.tokenlen);
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  bootId = esp_random();
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  Serial.print("BUTTON IP: ");
  Serial.println(WiFi.localIP());
  coap.server(onButton, "button");
  if (!coap.start()) Serial.println("GAGAL membuka UDP CoAP");
  Serial.println("Siap: GET /button. Tekan lalu lepas tombol.");
}

void loop() {
  const uint32_t now = millis();
  const int reading = digitalRead(BUTTON_PIN);
  if (reading != rawState) {
    rawState = reading;
    rawChangedAt = now;
  }
  if (rawState != stableState && now - rawChangedAt >= DEBOUNCE_MS) {
    stableState = rawState;
    if (stableState == LOW) {
      ++pressCount;
      Serial.printf("Tombol ditekan, hitungan: %lu\n", (unsigned long)pressCount);
    }
  }
  coap.loop();
  delay(1);
}
