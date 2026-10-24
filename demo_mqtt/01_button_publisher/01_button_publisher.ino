#include "NetworkConfig.h"
#include <esp_system.h>

const uint8_t BUTTON_PIN = 18;
const uint32_t DEBOUNCE_MS = 35;
DemoConnection connection;
uint32_t bootId;
uint32_t pressCount = 0;
int rawState = HIGH;
int stableState = HIGH;
uint32_t rawChangedAt = 0;
uint32_t lastPublishAt = 0;
String pendingEvent;
bool countDirty = true;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  bootId = esp_random();
  connection.begin("button", TOPIC_BUTTON_STATUS);
  Serial.println("Tombol: GPIO18 -> tombol -> GND");
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
      char event[24];
      snprintf(event, sizeof(event), "%08lx:%lu", (unsigned long)bootId,
               (unsigned long)pressCount);
      pendingEvent = event;
      countDirty = true;
      lastPublishAt = now - 500;
      Serial.println("Tombol ditekan: " + pendingEvent);
    }
  }
  if (connection.maintain()) countDirty = true;
  if (connection.mqtt.connected() && millis() - lastPublishAt >= 500) {
    lastPublishAt = millis();
    if (pendingEvent.length() > 0) {
      // Kejadian tidak di-retain: subscriber baru tidak membunyikan bel lama.
      if (connection.mqtt.publish(TOPIC_PRESS, pendingEvent, false, 1)) {
        Serial.println("PUB QoS1 demo/button/press = " + pendingEvent);
        pendingEvent = "";
      }
    }
    if (countDirty && connection.mqtt.connected()) {
      if (connection.mqtt.publish(TOPIC_COUNT, String(pressCount), true, 1)) {
        countDirty = false;
      }
    }
  }
  delay(1);
}
