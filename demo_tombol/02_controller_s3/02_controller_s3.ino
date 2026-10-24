#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include "NetworkConfig.h"



// IP kedua server dikonfigurasi pada tab NetworkConfig.h.


const uint16_t COAP_PORT = 5683;
const uint32_t POLL_MS = 250;
const uint32_t RESPONSE_TIMEOUT_MS = 2000;

WiFiUDP udp;
Coap coap(udp);
bool baselineReady = false;
String lastObserved;
String pendingEvent;
bool pollPending = false;
bool putPending = false;
uint16_t pollMid;
uint16_t putMid;
uint32_t lastPollAt = 0;
uint32_t lastPutAt = 0;

String readPayload(const CoapPacket &packet) {
  String result;
  for (size_t i = 0; i < packet.payloadlen; ++i) result += (char)packet.payload[i];
  return result;
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

void onResponse(CoapPacket &packet, IPAddress ip, int port) {
  if (port != COAP_PORT || packet.type != COAP_ACK) return;
  const String payload = readPayload(packet);
  if (ip == BUTTON_IP && pollPending && packet.messageid == pollMid) {
    pollPending = false;
    if (packet.code != COAP_CONTENT || !validEvent(payload)) return;
    // Mulai dari kondisi sekarang; reset tombol tidak dianggap sebagai tekanan.
    if (!baselineReady || payload.substring(0, 8) != lastObserved.substring(0, 8)) {
      lastObserved = payload;
      baselineReady = true;
      Serial.println("Sinkronisasi tombol: " + payload);
    } else if (payload != lastObserved) {
      lastObserved = payload;
      pendingEvent = payload;
      lastPutAt = millis() - RESPONSE_TIMEOUT_MS;  // Kirim kejadian baru segera.
      Serial.println("Tekanan baru: " + payload);
    }
  }
  if (ip == RECEIVER_IP && putPending && packet.messageid == putMid) {
    putPending = false;
    if (packet.code == COAP_CHANGED && payload == pendingEvent) {
      Serial.println("ACK 2.04: penerima menerima " + payload);
      pendingEvent = "";
    } else {
      Serial.println("Respons penerima tidak sesuai; akan dicoba lagi.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  Serial.print("CONTROLLER IP: ");
  Serial.println(WiFi.localIP());
  coap.response(onResponse);
  if (!coap.start()) Serial.println("GAGAL membuka UDP CoAP");
  Serial.println("Menunggu sinkronisasi tombol sebelum demo.");
}

void loop() {
  coap.loop();
  const uint32_t now = millis();
  if (pollPending && now - lastPollAt >= RESPONSE_TIMEOUT_MS) {
    pollPending = false;
    Serial.println("GET /button timeout: cek IP, Wi-Fi, dan daya board.");
  }
  if (putPending && now - lastPutAt >= RESPONSE_TIMEOUT_MS) {
    putPending = false;
    Serial.println("PUT /bell timeout: mencoba lagi dengan ID kejadian yang sama.");
  }
  if (WiFi.status() == WL_CONNECTED) {
    // Satu kejadian diproses sampai ACK; tekanan tambahan tetap dihitung server.
    if (!pollPending && !putPending && pendingEvent.length() == 0 &&
        now - lastPollAt >= POLL_MS) {
      pollMid = coap.get(BUTTON_IP, COAP_PORT, "button");
      pollPending = true;
      lastPollAt = now;
    }
    if (pendingEvent.length() > 0 && !putPending &&
        now - lastPutAt >= RESPONSE_TIMEOUT_MS) {
      putMid = coap.put(RECEIVER_IP, COAP_PORT, "bell", pendingEvent.c_str());
      putPending = true;
      lastPutAt = now;
      Serial.println("PUT /bell -> " + pendingEvent);
    }
  }
  delay(1);
}
