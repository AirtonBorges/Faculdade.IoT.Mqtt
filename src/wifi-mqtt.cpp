// wifi-mqtt.cpp
// Written by Wong SS
// 2024-01-20 tidied up to have settings in prj.h only
// 2023-11-02 updated to test with another broker
// 2023-10-31 First created

// https://wokwi.com/projects/379930552025347073

// This file has 2 sections: mqtt & wifi codes
// WiFi codes is event driven, thus non-blocking

// Red LED can be controlled by PB1 or MQTT. See below on MQTT.

// Use e.g. mqtt explorer (https://mqtt-explorer.com/) to connect to broker.emqx.io and 
// Publich topic xxxx/led with values of "toggleR" or "toggleG" to control the 
// Red or Green LED on/off. IMPORTANT: Do not send the double quotes!
#include "prj.h" // defines MQTT_IN, WOKWI and MQTT_ROOT_TOPIC
#include <WiFi.h>
#include <esp_wifi_types.h>
#include <Arduino.h>
#include "PubSubClient.h"
#include "dly-io.h"

static const char* mqttServer = MQTT_SERVER;
static char ledCmdTopic[] = TOPIC_LED_CMD;
static char strLedToggleR[] = PAYLOAD_TOGGLE_R;
static char strLedToggleG[] = PAYLOAD_TOGGLE_G;
static int port = 1883; // non-secured
// ========================================================================================================
// Functions in leds.cpp used by MQTT message
void ledR_toggle();  // toggle Red LED on/off
void ledG_toggle();  // toggle Green LED on/off
// ========================================================================================================
// From MQTT section of the codes
void mqttInit();
void mqttReconnect();
bool isMQTTok();
void chkMQTT();  // at ? interval, if WiFi connection OK and MQTT connection not ok, reconnect
// ========================================================================================================
// For WiFi section of the codes
// expects 
#define WIFICONNEV_DEBUG 1 // 1 for callbacks to print debug msgs

const char* ssid = USE_SSID;
const char* password = USE_PASSWORD;

void wifiSetup();  // call this in setup()
void wifiConnect(); // does WiFi.begin() if not connected
bool wifiConnOK(); // check if connection OK now
// void printWiFiInfo();
IPAddress getIP(); // get its IP address; if not connected, get 0.0.0.0

// USER MUST PROVIDE these functions called when at WiFi connection & disconnected
void doWhenWiFiConnected();
void doWhenWiFiDisonnected();
// ========================================================================================================
void wifiMQTTInit() {
  wifiSetup(); // comes back only if there's wifi connection
  mqttInit();
}
// ========================================================================================================
// MQTT codes
// ------------------------------------
void chkMQTT();  // this MUST be called in the loop() regularly
bool mqttSend(char topic[], char value[]);   // false if mqtt connection not ok

char mqttClientId[55];

WiFiClient espClient;
PubSubClient mqttClient(espClient);
static bool mqttConnOK = false;
// ------------------------------------
bool MQTT_ok() {
  return mqttConnOK;
}
// ------------------------------------
static SDELAYMS_T mqttChkDly(15000); // check MQTT connection status at intervals
void chkMQTT() {
  static uint16_t counter = 0;
  mqttClient.loop();
  if (mqttChkDly.dlyExpiredRestart()) {
    Serial.printf("\nChk MQTT status #%d: ",counter++);
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("WiFi status OK. ");
      if (!mqttClient.connected()) {
        Serial.print("MQTT conn not ok.");
        mqttReconnect();
      }
      else {
        Serial.print("MQTT conn ok");
      }
    }
    else {
      // shall we try to connect? i.e. instead of waiting for WiFi event ...
      Serial.print("WiFi conn NOT ok");
      wifiConnect();
    }
  }
}
// ------------------------------------
static void mqttCallback(char* topic, byte* message, unsigned int length) {
  Serial.print("\nMessage arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String stMessage;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    stMessage += (char)message[i];
  }
  if (String(topic) == ledCmdTopic) {
    // Topic subscribed. Compare message with expected values
    if(stMessage == strLedToggleR) {
      Serial.print("\nMQTT on/off Red LED");
      ledR_toggle();
    }
    else if(stMessage == strLedToggleG) {
      Serial.print("\nMQTT on/off Green LED");
      ledG_toggle();
    }
    else {
      Serial.print("???");
      Serial.print("\nUknown value: ");
      Serial.print(stMessage);
    }
  }
  else {
    Serial.print("Topic not handled");
  }
}
// ------------------------------------
bool mqttPublish(char topic[], char value[]) {
  if (mqttClient.connected()) {
    if (mqttClient.publish(topic, value)) { // publish topic, payload
      // Serial.print("\nPublishing done");
      return true;                              // publish success
    }
  }
  Serial.print("\nPublishing encountered error");
  return false;                               // publish failure
}
// ------------------------------------
void mqttInit() {
  Serial.printf("\nInstructions:\nConnect client to broker %s port %d.",mqttServer,port);
  Serial.printf("\nPublish topic %s of values %s or %s to on/off"
    "\nRed or Green LED",ledCmdTopic,strLedToggleR,strLedToggleG);
  Serial.printf("\n\nMQTT Setup: server %s, port %d & the callback",mqttServer,port); 
  mqttClient.setServer(mqttServer, port);
  mqttClient.setCallback(mqttCallback);
}
// ------------------------------------
// This assumes that WiFi connction is OK
void mqttReconnect() {
  long r = random(1000);
  sprintf(mqttClientId, "mqttClientId-%ld", r);
  Serial.printf("\nAttempting MQTT connection to %s...",mqttClientId);
  if (mqttClient.connect(mqttClientId)) {   // a random client ID
    Serial.print(" connected");
    Serial.printf("\nSend values %s %s to topic %s to on/off leds.", 
      strLedToggleR, strLedToggleG, ledCmdTopic);
    mqttClient.subscribe(ledCmdTopic);
    mqttConnOK = true;
  }
  else {
    mqttConnOK = false;
    Serial.printf(" Failed, rc=%d", mqttClient.state());
  }
}
// ------------------------------------
// Called by WiFi function below when WiFi connection is successful 
void doWhenWiFiConnected() {
  Serial.print("\nCalled by function at WiFi connected event");
  mqttReconnect();
}
// ------------------------------------
// Called by WiFi function below when WiFi is disconnected 
void doWhenWiFiDisonnected() {
  Serial.print("\nCalled by function at WiFi disconnected event");
  mqttConnOK = false;
}
// ========================================================================================================
// WiFi codes below
#if (WIFICONNEV_DEBUG)
static bool disconnectReported = false;
#endif
static bool wifiConnStatus = false;
static IPAddress myIP = (0, 0, 0, 0);
// ----------------------------------------
static void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
static void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
static void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info);
// ----------------------------------------
bool wifiConnOK() {
  return wifiConnStatus; // return a copy
}
// ----------------------------------------
void wifiConnect() {
  // see https://www.arduino.cc/reference/en/libraries/wifi/wifi.begin/
  if (WiFi.status() != WL_CONNECTED) {
    #if (WOKWI==1)  // defined in prj.h
      WiFi.begin(ssid, password, 6);
    #else
      WiFi.begin(ssid, password);
    #endif
      wifiConnStatus = false;
      myIP = (0, 0, 0, 0);
  }
}
// ----------------------------------------
// This does not wait for
void wifiSetup() {
  WiFi.disconnect(true); // delete old config
  WiFi.mode(WIFI_STA);
  delay(500);            // is this really necesssary?
  wifiConnStatus = false;
  // WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  wifiConnect();
#if (WIFICONNEV_DEBUG)
  Serial.printf("\nwifiSetup() called to connect to \"%s\""
                "\nCallbacks setup for: "
                // "\nEVENT_WIFI_STA_CONNECTED, EVENT_WIFI_STA_GOT_IP, EVENT_WIFI_STA_DISCONNECTED",
                "EVENT_WIFI_STA_GOT_IP, EVENT_WIFI_STA_DISCONNECTED",
                ssid);
#endif
}
// ----------------------------------------
IPAddress getIP() {
  return myIP;  // return a copy
}
// ----------------------------------------
static void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
#if (WIFICONNEV_DEBUG)
  Serial.printf("\n-- EVENT_WIFI_STA_CONNECTED callback invoked."
                "\nConnected to AP \"%s\" successfully!", ssid);
  // have not get IP yet, not consider connection complete
#endif
}
// ----------------------------------------
static void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  myIP = WiFi.localIP();
  wifiConnStatus = true;
#if (WIFICONNEV_DEBUG)
  Serial.printf("\n-- EVENT_WIFI_STA_GOT_IP callback invoked"
                "\nWiFi connected to AP \"%s\" with IP: ", ssid);
  Serial.print(myIP);

  Serial.print(" Mac addr: ");
  Serial.print(WiFi.macAddress());
  disconnectReported = false;
#endif
  doWhenWiFiConnected(); // 
}
// ----------------------------------------
// WiFi disconnection handling
static void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
#if (WIFICONNEV_DEBUG)
  if (!disconnectReported) {
    Serial.printf("\n-- EVENT_WIFI_STA_DISCONNECTED callback invoked"
                  "\nDisconnected from WiFi AP \"%s\"", ssid);

    Serial.print("\nWiFi lost connection. Reason: ");
    switch (info.wifi_sta_disconnected.reason) {
      case wifi_err_reason_t::WIFI_REASON_NO_AP_FOUND:
        Serial.print("NO_AP_FOUND");
        break;
      case wifi_err_reason_t::WIFI_REASON_AUTH_LEAVE:
        Serial.print("AUTH_LEAVE. Probably AP down");
        break;
      default: // other reasons
        Serial.print(info.wifi_sta_disconnected.reason);
    }
    Serial.print("\nTrying to Reconnect");
  }
  disconnectReported = true; // disconnection reported, don't report again
#endif
  wifiConnect();
  doWhenWiFiDisonnected();
}
// ----------------------------------------