#include "NetworkConfig.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int OLED_SDA = 8;
const int OLED_SCL = 9;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
DemoConnection connection;
bool oledReady = false;
String pressCount = "?";
String ledState = "?";
String buttonStatus = "?";
String receiverStatus = "?";
String lastEvent = "Belum ada";
uint32_t lastDrawAt = 0;

void onMessage(String &topic, String &payload) {
  Serial.println("SUB " + topic + " = " + payload);
  if (topic == TOPIC_PRESS && validEvent(payload)) lastEvent = payload;
  else if (topic == TOPIC_COUNT && payload.length() > 0 && payload.length() <= 10) {
    for (unsigned int i = 0; i < payload.length(); ++i) {
      if (payload[i] < '0' || payload[i] > '9') return;
    }
    pressCount = payload;
  } else if (topic == TOPIC_LED && (payload == "ON" || payload == "OFF")) {
    ledState = payload;
  } else if (topic == TOPIC_BUTTON_STATUS) buttonStatus = payload;
  else if (topic == TOPIC_LED_STATUS) receiverStatus = payload;
}

void drawScreen() {
  if (!oledReady) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  display.setCursor(0, 0);
  display.print("MQTT BEL - SSD1306");
  display.setCursor(0, 12);
  display.print("WiFi:");
  display.print(WiFi.status() == WL_CONNECTED ? "OK" : "--");
  display.print(" MQTT:");
  display.print(connection.mqtt.connected() ? "OK" : "--");
  display.setCursor(0, 24);
  display.print("Tekanan: ");
  display.print(pressCount);
  display.setCursor(0, 36);
  display.print("LED: ");
  display.print(receiverStatus == "online" ? ledState : "?");
  display.setCursor(0, 46);
  display.print("B:");
  display.print(buttonStatus == "online" ? "ON" : (buttonStatus == "offline" ? "OFF" : "?"));
  display.print(" R:");
  display.print(receiverStatus == "online" ? "ON" : (receiverStatus == "offline" ? "OFF" : "?"));
  display.setCursor(0, 56);
  display.print(lastEvent);
  display.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(100000);
  Wire.setTimeOut(20);
  uint8_t address = 0;
  for (uint8_t candidate : {0x3C, 0x3D}) {
    Wire.beginTransmission(candidate);
    if (Wire.endTransmission() == 0) {
      address = candidate;
      break;
    }
  }
  if (address != 0) oledReady = display.begin(SSD1306_SWITCHCAPVCC, address, true, false);
  if (oledReady) Serial.printf("OLED ditemukan: 0x%02X, SDA=%d SCL=%d\n", address, OLED_SDA, OLED_SCL);
  else Serial.println("OLED gagal: cek 3V3/GND/SDA8/SCL9. Monitor MQTT tetap berjalan di Serial.");
  connection.mqtt.onMessage(onMessage);
  connection.begin("oled", "demo/oled/status");
  drawScreen();
}

void loop() {
  if (connection.maintain()) {
    if (!connection.mqtt.subscribe("demo/button/#", 1) ||
        !connection.mqtt.subscribe("demo/led/#", 1)) {
      Serial.println("Subscribe gagal; menyambung ulang.");
      connection.mqtt.disconnect();
    }
  }
  if (millis() - lastDrawAt >= 200) {
    lastDrawAt = millis();
    drawScreen();
  }
  delay(1);
}
