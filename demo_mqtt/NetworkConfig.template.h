#pragma once
#include <WiFi.h>
#include <MQTT.h>

// Salinan konfigurasi ada di setiap folder sketch agar bisa dibuka langsung.
const char *WIFI_SSID = "GANTI_NAMA_WIFI";
const char *WIFI_PASSWORD = "GANTI_PASSWORD_WIFI";
const char *MQTT_HOST = "GANTI_IP_LAPTOP";
const uint16_t MQTT_PORT = 1883;

const char *TOPIC_PRESS = "demo/button/press";
const char *TOPIC_COUNT = "demo/button/count";
const char *TOPIC_LED = "demo/led/state";
const char *TOPIC_BUTTON_STATUS = "demo/button/status";
const char *TOPIC_LED_STATUS = "demo/led/status";

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

class DemoConnection {
 public:
  WiFiClient transport;
  MQTTClient mqtt{256};
  String clientId;
  String statusTopic;
  uint32_t lastAttempt = 0;

  void begin(const char *role, const char *availabilityTopic) {
    char suffix[16];
    const uint64_t chipId = ESP.getEfuseMac();
    snprintf(suffix, sizeof(suffix), "%04x%08lx", (unsigned int)(chipId >> 32),
             (unsigned long)(uint32_t)chipId);
    clientId = String(role) + "-" + suffix;
    statusTopic = availabilityTopic;
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    mqtt.begin(MQTT_HOST, MQTT_PORT, transport);
    mqtt.setOptions(10, true, 1000);
    mqtt.setWill(statusTopic.c_str(), "offline", true, 1);
    lastAttempt = millis() - 5000;
    Serial.printf("Wi-Fi: %s | Broker: %s:%u\n", WIFI_SSID, MQTT_HOST, MQTT_PORT);
    Serial.println("Client ID: " + clientId);
  }

  // true hanya pada koneksi MQTT baru: sketch dapat subscribe ulang.
  bool maintain() {
    if (WiFi.status() != WL_CONNECTED) return false;
    mqtt.loop();
    if (mqtt.connected() || millis() - lastAttempt < 5000) return false;
    lastAttempt = millis();
    Serial.print("Menghubungkan broker dari IP ");
    Serial.println(WiFi.localIP());
    if (!mqtt.connect(clientId.c_str())) {
      Serial.printf("MQTT gagal: error=%d, return=%d. Cek IP broker/port/firewall.\n",
                    (int)mqtt.lastError(), (int)mqtt.returnCode());
      return false;
    }
    Serial.println("MQTT terhubung");
    if (!mqtt.publish(statusTopic, "online", true, 1)) {
      mqtt.disconnect();
      return false;
    }
    return true;
  }
};
