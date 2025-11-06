// prj.h
#ifndef _PROJ__H
#define _PROJ__H
// 2024-02-28 another tidying up
// 2024-02-11 slight refactoring
// 2024-01-20 tidied up to have settings in prj.h only
// 2023-11-02 updated to test with another broker
// 2023-10-31 First created

// IMPORTANT:
// 1. Make a copy of this project in your own wokwi free account.
// 2. MUST set these values correctly:
//  a. Change MQTT_ROOT_TOPIC name so that it will not clash with others running the same program
//  b. MQTT_IN = 1 if WiFi and MQTT used, 0 they are not used
//  c. WOKWI = 1 uses WOKWI WiFi simulator, 0 use own actual ESP32 hardware upload by Arduino IDE

// Files:
// test-dly-io-mqtt.ino - setup() and loop() ...
// prj.h                - hardware connections
// dly-io.h             - C++ classes SDELAYMS_T, SINPUT_T and SOUTPUT_T definition
// dly-io.cpp           - C++ classes implementations
// leds.cpp             - functions for LEDs; updated to send mqtt msgs on led statuses
// wifi-mqtt.cpp        - wifi and mqtt functions

// Program description is in the "led-pb-mqtt.ino" file.
// ====================================================================
// Do update info below
#define MQTT_IN 1  // 1 if WiFi and MQTT used on top of PB1/2 and Serial cmds
#define WOKWI   1  // 1 to simulate in WOKWI, 0 for compile in Arduino & upload to own ESP32 HW
// If MQTT_IN is 1, change MQTT_ROOT_TOPIC to be unique to you so that it 
// will not clash with others running this same program
#define MQTT_ROOT_TOPIC "leds" // MAKE THIS UNIQUE e.g. "s12345678" my student no

#define MQTT_SERVER "broker.hivemq.com"    // make sure remote app uses the same MQTT server
// #define MQTT_SERVER "broker.emqx.io"    // this server works too

// SSID ... below are USED ONLY IF WOKWI is 0 and the program will be compiled in Arduino IDE
// & uploaded to your actual esp32 hardware
#define MY_SSID      "change to own ssid" // used if not simulating in WOKWI; use own hardware
#define MY_PASSWORD  "change your ssid pwd"

// ====================================================================
// NO NEED TO CHANGE ANYTHING BELOW
#define WOKWI_SSID      "Wokwi-GUEST"   // used if WOKWI=1 for WiFi simulation in Wokwi 
#define WOKWI_PASSWORD  ""

#if (WOKWI==1) // 1 = wifi simulate in WOKWI, 0 = actual esp32 wifi used
#define USE_SSID      WOKWI_SSID // for Wokwi simulation
#define USE_PASSWORD  WOKWI_PASSWORD
#else       // actual wifi hardware in ESP32 IoT Kit
#define USE_SSID      MY_SSID    // change to your router/hotspot
#define USE_PASSWORD  MY_PASSWORD
#endif

#define TOPIC_LED_STATUS_Y   MQTT_ROOT_TOPIC "/ledy"
#define TOPIC_LED_STATUS_G   MQTT_ROOT_TOPIC "/ledg"
#define TOPIC_LED_STATUS_R   MQTT_ROOT_TOPIC "/ledr"

#define TOPIC_LED_CMD        MQTT_ROOT_TOPIC "/led"
#define PAYLOAD_TOGGLE_R     "toggleR"
#define PAYLOAD_TOGGLE_G     "toggleG"

#include <Arduino.h>

#ifndef MQTT_IN
  #error You must define MQTT_IN 1 or 0 in prj.h
#endif
#ifndef WOKWI
  #error You must define WOKWI 1 or 0 in prj.h
#endif

// The pin numbers and on/press values are the same as NP-SoE IoT Kitset
#define ledPinR   23
#define ledOnValR LOW
#define ledPinG   18
#define ledOnValG LOW
#define ledPinY   19
#define ledOnValY LOW

#define ledOnVal  LOW  // General

#define PB1       27
#define PB2       25
#define PBPressVal 0   // General

#endif