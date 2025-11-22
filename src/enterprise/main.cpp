/*
  MQTT example (WPA/WPA2-Enterprise) for ESP8266 (ESP12E)
  - Versão modificada para conectar em redes WPA/WPA2-Enterprise
  - Atenção: suporte WPA3-Enterprise pode não estar disponível no core ESP8266
  - Identity/Username/Password são configurados abaixo

  Credenciais fornecidas pelo usuário:
  Identity/Username: id_pessoa_univali_aqui
  Password: senha_univali_aqui
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>

// IntelliSense stub: evita erro de 'no member named setEnterprisePassword' no editor
#ifdef __INTELLISENSE__
class ESP8266WiFiClass {
public:
  void beginEnterprise(const char* ssid) {}
  void setEnterpriseIdentity(const char* id) {}
  void setEnterpriseUsername(const char* user) {}
  void setEnterprisePassword(const char* pass) {}
};
extern ESP8266WiFiClass WiFi;
#endif

// WiFi (WPA/WPA2-Enterprise)
const char* ssid = "Caramelo";
const char* ent_identity = "id_pessoa_univali_aqui"; // identity (também usado como username aqui)
const char* ent_username = "id_pessoa_univali_aqui";
const char* ent_password = "senha_univali_aqui";

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
    digitalWrite(LED_BUILTIN, LOW); // liga (ativo LOW)
    Serial.println("LED ligado");
  } else if (low == "não" || low == "nao" || low == "não") {
    digitalWrite(LED_BUILTIN, HIGH); // desliga
    Serial.println("LED desligado");
  } else {
    Serial.println("Comando não reconhecido");
  }
}

void connectWiFiEnterprise() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  Serial.print("Conectando ao WiFi (WPA/WPA2-Enterprise) ");
  
#if defined(WIFI_ESP_ENTERPRISE_SUPPORT)
  WiFi.beginEnterprise(ssid);
  WiFi.setEnterpriseIdentity(ent_identity);
  WiFi.setEnterpriseUsername(ent_username);
  WiFi.setEnterprisePassword(ent_password);
#else
  Serial.println("Aviso: API WPA/WPA2-Enterprise não disponível no core atual.");
  Serial.println("Atualize o framework ESP8266 ou habilite suporte enterprise se necessário.");
  // Como fallback apenas chamamos begin (pode falhar para redes enterprise)
  WiFi.begin(ssid);
#endif

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 30000) {
      Serial.println("\nFalha ao conectar ao WiFi (Enterprise), tentando novamente...");
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
  // Mantemos 74880 para facilitar leitura do bootloader + rotina inicial
  Serial.begin(74880);
  delay(10);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  connectWiFiEnterprise();

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
