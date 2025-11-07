// led-pb-mqtt.ino
// Written by Wong SS

// Refer to the info & VERY IMPORTANT instructions in prj.h

// Program description is as below.
// ===========================================================================================
// What this program does:
// 1. Yellow LED always on for 200ms and off for 1800ms as system heartbeat
// 2. Both Red and Green LEDs can be on/off (toggle):
// - by pressing PB1 & PB2 respectively
// - At the serial monitor, send 'r' or 'g'
// - If WiFi & MQTT used send appropriate topic and payload described in section below
// The WiFi can be simulated in Wokwi or can use actual ESP32 hardware with program
// uploaded thru Arduino

// This program is non-blocking even if WiFi & MQTT used and their connections fail.
// Reason: The WiFi connection, disconnections handling (reconnection) are all event driven.
// However, an attempt to connect to MQTT server may take a little bit of time
// ===========================================================================================
// If MQTT_IN==1:
// Red and Green LED can also be controlled by remote app that sends/receive MQTT messages
// with the MQTT server indicated in prj.h
// 1. On Windows, can use e.g. mqtt explorer (https://mqtt-explorer.com/) to monitor/send
//    MQTT messages
// 2. Android mobile app "IoT MQTT Panel" to monitor/control the LEDs remotely.

// Remote app should publish to control and subscribe to monitor as below
// PUBLISH (replace xxxx with the root name you use in prj.h)
// "xxxx/led" with values "toggleR" or "toggleG"
// to on/off  Red & GReen LED ing. IMPORTANT: NOT double quotes!

// SUBSCRIBE topics that has values of "1" or "0" (sent by the system every 2s)
// "xxxx/ledy" - toggle "1" and "0" at regularly indicating system alive
// "xxxx/ledr" - "1" means Red is on, "0" disabled
// "xxxx/ledg" - "1" means Green is on, "0" disabled
// ===========================================================================================
#include "prj.h" // defines MQTT_IN, WOKWI and MQTT_ROOT_TOPIC
#include "dly-io.h" // C++ classes: SDELAYMS_T, SINPUT_T, SOUTPUT_T
// ===========================================================================================
#if (MQTT_IN)  // defined in prj.h
// Functions in mqtt.cpp
void wifiMQTTInit();
void chkMQTT();
#endif
// ===========================================================================================
// Functions in leds.cpp
void ledInit();                 // call once at the beginning
void ledG_toggle(); // toggle Green LED on/off
void ledR_toggle(); // toggle Red LED on/off
void blinkLedY();
void mqttRptLEDs(); // will send MQTT message regularly on LED status
// ===========================================================================================
SINPUT_T pb1(PB1,INPUT,200,LOW); // pin, inputType, bounceDuration, PressedIsLOW
SINPUT_T pb2(PB2,INPUT,200,LOW);
// ------------------------------------
void chkPBs() {
  if (pb1.justPressed()) {
    ledR_toggle();
  }
  if (pb2.justPressed()) {
    ledG_toggle();
  }
}
// ------------------------------------
void setup() {
  delay(100);
  Serial.begin(115200);
  delay(100);
  Serial.print("\nDemo:"
    "\nRed & Green LEDs on/off controlled by PB1 & PB2, or Serial 'r' or 'g' cmds"
    "\nYellow LED always pulse at 0.2s on and 1.8s off as system heartbeat");
#if (MQTT_IN)
  Serial.print("\n\nMQTT ...."
    "\nIMPORTANT: Make a copy of this project in your own wokwi free account"
    "\nIn prj.h, change the MQTT_ROOT_TOPIC name to be unique to you"
    "\nso that it will not clash with others running the same program"
    "\nDo the same for running in your ESP32 hardware.");
  wifiMQTTInit();
#endif
  ledInit();
  Serial.print("\nEnd of setup()\n");
}
// ------------------------------------
void chkSerial() {
  if (Serial.available()) {
    switch (Serial.read()) {
      case 'r': case 'R':
        ledR_toggle();
        break;
      case 'g': case 'G':
        ledG_toggle();
        break;
    }
  }
}
// ------------------------------------
void loop() {
  blinkLedY();
  chkPBs();
  chkSerial();
#if (MQTT_IN)
  chkMQTT();      // Must be called very regularly to service MQTT msgs
  mqttRptLEDs();  // send MQTT msg at regular intervals reporting LED status
#endif
}
// ------------------------------------
