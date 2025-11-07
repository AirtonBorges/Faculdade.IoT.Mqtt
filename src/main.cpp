#include "DHT.h"
#include <DHT.h>
#include <Arduino.h>

// Constants
#define DHTPIN 14
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

void setup()
{
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("boot");
  delay(100);
  // dht.begin();
}

void loop()
{
  // float t = dht.readTemperature();
  // float h = dht.readHumidity();

  // // Check if any reads failed and exit early (to try again).
  // if (isnan(h) || isnan(t)) {
  Serial.println(F("Teste!"));
  //   return;
  // }
  // // Compute heat index in Celsius (isFahreheit = false)
  // float hic = dht.computeHeatIndex(t, h, false);
  digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
  delay(1000);                     // wait for a second
  digitalWrite(LED_BUILTIN, LOW);  // turn the LED off by making the voltage LOW
  delay(1000);
}
