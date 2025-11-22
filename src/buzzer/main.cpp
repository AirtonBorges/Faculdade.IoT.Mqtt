// Faz com que o buzzer emita um som intermitente a cada 5 segundos durante 10 ms
// Conecte o buzzer ao pino definido em RELAY_PIN

#include <Arduino.h>

#define PINO 2 // D4 em NodeMCU

// The setup function runs once on reset or power-up
void setup() {
  pinMode(PINO, OUTPUT);
}

// The loop function repeats indefinitely
void loop() {
  digitalWrite(PINO, HIGH); // Buzzer desligado
  delay(5000);
  digitalWrite(PINO, LOW); // Buzzer ligado
  delay(30);
}
