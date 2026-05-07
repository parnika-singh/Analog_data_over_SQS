#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "secrets.h"

WiFiClientSecure net;
PubSubClient client(net);

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int retries = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    retries++;

    if (retries > 20) {
      Serial.println("\n❌ WiFi Failed!");
      Serial.print("Status code: ");
      Serial.println(WiFi.status());
      break;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }
}

void connectAWS() {
  Serial.println("\n🔐 Setting certificates...");

  net.setCACert(ROOT_CA);
  net.setCertificate(DEVICE_CERT);
  net.setPrivateKey(PRIVATE_KEY);

  client.setServer(AWS_IOT_ENDPOINT, 8883);

  Serial.println("🌐 Connecting to AWS IoT Core...");

  int retries = 0;

  while (!client.connected()) {
    Serial.print("Attempt ");
    Serial.println(retries + 1);

    if (client.connect("ESP32Client-1")) {
      Serial.println("✅ Connected to AWS IoT Core!");
    } else {
      Serial.print("❌ Failed, rc=");
      Serial.println(client.state());
      Serial.println("Retrying in 2 seconds...");
      delay(2000);
      retries++;

      if (retries > 10) {
        Serial.println("❌ AWS IoT connection failed after retries");
        return;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n🚀 ESP32 Booting...");

  connectWiFi();
  connectAWS();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi disconnected! Reconnecting...");
    connectWiFi();
  }

  if (!client.connected()) {
    Serial.println("⚠️ AWS IoT disconnected! Reconnecting...");
    connectAWS();
  }

  client.loop();

  int value = analogRead(34);

  float voltage = (value / 4095.0) * 3.3;

  char payload[100];
  sprintf(payload, "{\"voltage\": %.2f, \"value\": %d}", voltage, value);

  Serial.print("📤 Publishing: ");
  Serial.println(payload);

  if (client.publish("esp32/data", payload)) {
    Serial.println("✅ Publish success");
  } else {
    Serial.println("❌ Publish failed");
  }

  Serial.println("----------------------------------");

  delay(8000);
}