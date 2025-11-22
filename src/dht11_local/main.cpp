/*
  DHT11 local reader for ESP8266
  - Versão sem WiFi/MQTT: apenas lê o DHT11 e imprime temperatura/umidade a cada 10 segundos
  - Ajuste `DHTPIN` conforme seu cabeamento
*/

#include <Arduino.h>
#include <DHT.h>
#include <math.h>

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

  // tolerâncias para considerar "mudança" entre leituras
  const float TEMP_EPS = 0.1f; // 0.1 °C
  const float HUM_EPS = 0.1f;  // 0.1 %

  static float prevT = NAN;
  static float prevH = NAN;

  if (isnan(h) || isnan(t)) {
    Serial.println("Falha ao ler o DHT11");
  } else {
    bool changed = false;

    if (isnan(prevT) || fabsf(t - prevT) >= TEMP_EPS) {
      changed = true;
    }
    if (isnan(prevH) || fabsf(h - prevH) >= HUM_EPS) {
      changed = true;
    }

    if (changed) {
      Serial.print("Temperatura: ");
      Serial.print(t, 1);
      Serial.print(" °C    Umidade: ");
      Serial.print(h, 1);
      Serial.println(" %");

      // atualiza valores anteriores
      prevT = t;
      prevH = h;
    }

    // Controle do LED: acende se umidade > 75%, caso contrário apaga
    if (h > 75.0) {
      digitalWrite(LED_BUILTIN, LOW); // liga (LED normalmente ativo LOW)
    } else {
      digitalWrite(LED_BUILTIN, HIGH); // desliga
    }
  }

  // aguarda 1 segundo antes da próxima leitura
  delay(1000);
}
