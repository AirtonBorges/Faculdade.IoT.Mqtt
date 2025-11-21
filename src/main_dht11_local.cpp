/*
  DHT11 local reader for ESP8266
  - Versão sem WiFi/MQTT: apenas lê o DHT11 e imprime temperatura/umidade a cada 10 segundos
  - Ajuste `DHTPIN` conforme seu cabeamento
*/

#include <Arduino.h>
#include <DHT.h>

// DHT11
#define DHTPIN D5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(74880);
  delay(10);
  Serial.println();
  Serial.println("DHT11 local reader - iniciando");
  dht.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // LED desligado por padrão (ativa LOW em muitas placas)
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Falha ao ler o DHT11");
  } else {
    Serial.print("Temperatura: ");
    Serial.print(t, 1);
    Serial.print(" °C    Umidade: ");
    Serial.print(h, 1);
    Serial.println(" %");
    // Controle do LED: acende se umidade > 75%, caso contrário apaga
    if (h > 75.0) {
      digitalWrite(LED_BUILTIN, LOW); // liga (LED normalmente ativo LOW)
      Serial.println("LED: ON (umidade > 75%)");
    } else {
      digitalWrite(LED_BUILTIN, HIGH); // desliga
      Serial.println("LED: OFF (umidade <= 75%)");
    }
  }

  // aguarda 10 segundos antes da próxima leitura
  delay(10000);
}
