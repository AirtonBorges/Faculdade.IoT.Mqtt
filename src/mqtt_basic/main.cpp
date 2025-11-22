/*
  MQTT example for ESP8266 (ESP12E)
  - Conecta ao WiFi
  - Conecta ao broker MQTT
  - Se receber mensagem "sim" no tópico `solfo_IoT_univali` -> liga o LED embutido
  - Se receber mensagem "não" ou "nao" -> desliga o LED

  Configurações fornecidas pelo usuário:
  SSID: Seu_wifi_aqui
  Senha: senha_do_wifi_aqui
  Broker: test.mosquitto.org:1883
  Tópico: topico_mqtt_aqui
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>

// WiFi
const char* ssid = "Seu_wifi_aqui";
const char* password = "senha_do_wifi_aqui";

// MQTT
const char* mqtt_server = "test.mosquitto.org";
const uint16_t mqtt_port = 1883;
const char* mqtt_topic = "topico_mqtt_aqui";

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

  if (low == "sim") {
    // No ESP8266 o LED_BUILTIN normalmente é ativo LOW
    digitalWrite(LED_BUILTIN, LOW); // liga
    Serial.println("LED ligado");
  } else if (low == "não" || low == "nao" || low == "não") {
    digitalWrite(LED_BUILTIN, HIGH); // desliga
    Serial.println("LED desligado");
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
    // usa MAC como client ID para evitar colisões
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
  // Ajustado para 74880 para combinar com as mensagens de boot do ESP8266
  // e com `monitor_speed` em platformio.ini
  Serial.begin(74880);
  delay(10);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // desligado por padrão (ativo LOW)

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
