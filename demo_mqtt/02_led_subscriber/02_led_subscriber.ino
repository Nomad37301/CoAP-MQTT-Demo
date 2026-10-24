#include "NetworkConfig.h"

const int LED_PIN = 18;
const bool LED_ACTIVE_HIGH = true;
const bool LED_BOOT_TEST = true;
const uint32_t FLASH_MS = 1000;
DemoConnection connection;
String lastEvent;
bool ledOn = false;
bool stateDirty = true;
uint32_t ledStartedAt = 0;
uint32_t lastStateAttempt = 0;

void setLed(bool on) {
  ledOn = on;
  if (LED_PIN >= 0) digitalWrite(LED_PIN, (on == LED_ACTIVE_HIGH) ? HIGH : LOW);
  stateDirty = true;
}

void onMessage(String &topic, String &payload) {
  if (topic != TOPIC_PRESS || !validEvent(payload) || payload == lastEvent) return;
  lastEvent = payload;
  setLed(true);
  ledStartedAt = millis();
  Serial.println("BEL MASUK: " + payload + " | LED ON");
  // Publish status dilakukan di loop, bukan di callback MQTT.
}

void setup() {
  Serial.begin(115200);
  if (LED_PIN >= 0) {
    pinMode(LED_PIN, OUTPUT);
    setLed(false);
    if (LED_BOOT_TEST) {
      Serial.printf("TES LED GPIO%d: dua kedipan\n", LED_PIN);
      for (int i = 0; i < 2; ++i) {
        setLed(true);
        delay(250);
        setLed(false);
        delay(250);
      }
    }
  }
  connection.mqtt.onMessage(onMessage);
  connection.begin("led", TOPIC_LED_STATUS);
}

void loop() {
  if (ledOn && millis() - ledStartedAt >= FLASH_MS) {
    setLed(false);
    Serial.println("LED OFF");
  }
  if (connection.maintain()) {
    if (!connection.mqtt.subscribe(TOPIC_PRESS, 1)) {
      Serial.println("Subscribe gagal; menyambung ulang.");
      connection.mqtt.disconnect();
    } else {
      stateDirty = true;
      Serial.println("SUB QoS1 demo/button/press");
    }
  }
  if (connection.mqtt.connected() && stateDirty && millis() - lastStateAttempt >= 200) {
    lastStateAttempt = millis();
    const char *state = ledOn ? "ON" : "OFF";
    stateDirty = false;
    if (!connection.mqtt.publish(TOPIC_LED, state, true, 1)) stateDirty = true;
    // Callback dapat terjadi selama publish QoS1; flag baru tetap dipertahankan.
  }
  delay(1);
}
