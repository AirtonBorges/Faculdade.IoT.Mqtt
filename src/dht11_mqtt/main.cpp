#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "env.h"

// Ajuste o pino conforme sua ligação
#define DHTPIN D5
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

void connectWiFi() {
  Serial.print("Conectando em ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
    if (millis() - start > 20000) {
      Serial.println("\nFalha em conectar WiFi, reiniciando...");
      ESP.restart();
    }
  }
  Serial.println("\nWiFi conectado. IP: " + WiFi.localIP().toString());
}

void reconnectMqtt() {
  while (!client.connected()) {
    Serial.print("Conectando MQTT ao broker ");
    Serial.print(MQTT_BROKER);
    Serial.print(":" );
    Serial.println(MQTT_PORT);
    if (client.connect("dht11_client_" WIFI_SSID)) {
      Serial.println("MQTT conectado");
    } else {
      Serial.print("Falha MQTT, rc=");
      Serial.print(client.state());
      Serial.println(". Tentando novamente em 5s");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);
  dht.begin();

  connectWiFi();

  client.setServer(MQTT_BROKER, atoi(MQTT_PORT));
  reconnectMqtt();
}

unsigned long lastPublish = 0;
const unsigned long publishInterval = 15000; // 15s

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (!client.connected()) {
    reconnectMqtt();
  }
  client.loop();

  if (millis() - lastPublish >= publishInterval) {
    lastPublish = millis();
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println("Falha ao ler DHT");
      return;
    }

    // Usar os tópicos completos definidos em .env
    String topicTemp = String(TOPICO_TEMPERATURA);
    String topicHum = String(TOPICO_UMIDADE);

    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", t);
    Serial.print("Publicando temperatura "); Serial.print(buf); Serial.print(" para "); Serial.println(topicTemp);
    client.publish(topicTemp.c_str(), buf);

    snprintf(buf, sizeof(buf), "%.1f", h);
    Serial.print("Publicando umidade "); Serial.print(buf); Serial.print(" para "); Serial.println(topicHum);
    client.publish(topicHum.c_str(), buf);
  }
}
