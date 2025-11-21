/*
  MQTT + DHT11 example for ESP8266 (ESP12E)
  - Quando receber mensagem "Enviar" (case-insensitive) no tópico `seu_topico_aqui`,
    o dispositivo lê o sensor DHT11 e publica temperatura/umidade no mesmo tópico.

  Ajuste o pino `DHTPIN` conforme necessário (ex.: D2, D3, etc. no NodeMCU).
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <DHT.h>

// WiFi
const char* ssid = "Nome__do_WIFI_aqui";
const char* password = "Senha_do_WIFI-aqui";

// MQTT
const char* mqtt_server = "test.mosquitto.org";
const uint16_t mqtt_port = 1883;
const char* mqtt_topic = "Seu_topico_aqui";

// DHT11
// No NodeMCU/ESP8266, pinos D0..D8 mapeiam para GPIO -- ajuste conforme seu cabeamento.
#define DHTPIN D5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  msg.trim();
  String low = msg;
  low.toLowerCase();

  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);

  if (low == "enviar") {
    // Ler DHT11 e publicar no mesmo tópico em formato JSON simples
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
      Serial.println("Falha ao ler o DHT11");
      return;
    }
    String payloadOut = "{";
    payloadOut += "\"temperature\":" + String(t, 1) + ",";
    payloadOut += "\"humidity\":" + String(h, 1);
    payloadOut += "}";

    Serial.print("Enviando: ");
    Serial.println(payloadOut);
    client.publish(mqtt_topic, payloadOut.c_str());
  } else {
    Serial.println("Comando não reconhecido");
  }
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 20000) {
      Serial.println("\nFalha ao conectar ao WiFi, tentando novamente...");
      start = millis();
    }
  }
  Serial.println();
  Serial.print("Conectado. IP: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    String clientId = "ESP8266-";
    clientId += WiFi.macAddress();
    if (client.connect(clientId.c_str())) {
      Serial.println("conectado");
      client.subscribe(mqtt_topic);
      Serial.print("Inscrito em: ");
      Serial.println(mqtt_topic);
    } else {
      Serial.print("falha, rc=");
      Serial.print(client.state());
      Serial.println("; tentando novamente em 5s");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(74880);
  delay(10);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  dht.begin();

  connectWiFi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
  connectMQTT();
}

void loop() {
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();
}
