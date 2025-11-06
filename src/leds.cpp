// leds.cpp
// Written by Wong SS
// 2024-01-20 tidied up to have settings in prj.h only
// 2023-11-02 updated to test with another broker
// 2023-10-31 First created

#include "prj.h" // defines MQTT_IN, WOKWI and MQTT_ROOT_TOPIC
#include "dly-io.h" // C++ classes: 
// ========================================================================================================
#define DUR_Y_ON    200   // Yellow LED on for xxx ms
#define DUR_Y_OFF   1800  // Yellow LED off for xxx ms
#define REPORT_INTV 1000  // MQTT status report interval

static SOUTPUT_T ledR(ledPinR,ledOnVal);  // pin and onValue. initially all LEDs off by default
static SOUTPUT_T ledY(ledPinY,ledOnVal);
static SOUTPUT_T ledG(ledPinG,ledOnVal);
// ========================================================================================================
// this section added for MQTT
// function in wifi-mqtt.cpp
#if (MQTT_IN)  // defined in prj.h
bool mqttPublish(char topic[], char value[]);
bool MQTT_ok();
void mqttRptLEDs(); // will send MQTT message regularly on LED status

char MQTT_ledVal_on[]  = "1";
char MQTT_ledVal_off[] = "0";
char ledStatusY[] = TOPIC_LED_STATUS_Y;
char ledStatusR[] = TOPIC_LED_STATUS_R;   // led on (1) or off (0)
char ledStatusG[] = TOPIC_LED_STATUS_G;
// char ledStatusY[] = MQTT_ROOT_TOPIC "/ledy";   // toggle 2 sec intervals indicating system alive
// char ledStatusR[] = MQTT_ROOT_TOPIC "/ledr";   // led on (1) or off (0)
// char ledStatusG[] = MQTT_ROOT_TOPIC "/ledg";
// ===========================================================================================
// Send MQTT msgs on LED status
static void mqttPublishYToggle() {
  static bool toggle = false;
  char *valptr;
  // Serial.print("\nLED mqtt pub");
  toggle = !toggle;
  mqttPublish(ledStatusY,toggle? MQTT_ledVal_on:valptr = MQTT_ledVal_off); // publish Yellow
}
// ------------------------------------
// to be called regularly to report LED status
void mqttRptLEDs() {
  static SDELAYMS_T rptDly(REPORT_INTV);
  if (rptDly.dlyExpiredRestart()) {
    if (!MQTT_ok()) return;
    Serial.print(" S");
    mqttPublishYToggle();
    mqttPublish(ledStatusR, ledR.isOn()? MQTT_ledVal_on : MQTT_ledVal_off);
    mqttPublish(ledStatusG, ledG.isOn()? MQTT_ledVal_on : MQTT_ledVal_off);
  }
}
#endif // MQTT_IN
// ===========================================================================================
void ledInit() {
  Serial.print("\nInitialize LEDs");
#if (MQTT_IN) // defined in prj.h
  // Serial.printf(
  //   "\n\nSubscribe topics to show LED on/off status. Value 1=on, 0=off"
  //   "\n%s\n%s",ledStatusR,ledStatusG);
  // Serial.printf(
  //   "\nIf MQTT conn ok, this topic published toggling between 1/0 at %.1fs intervals"
  //   "\n%s",float(REPORT_INTV)/1000.,ledStatusY);
#endif // MQTT_IN
}
// ===========================================================================================
// Green LED
// ------------------------------------
void reportLedG_status() {
  if (ledG.isOn()) {
    Serial.print("\nGreen LED on");
  } 
  else {
    Serial.print("\nGreen LED off");
  } 
}
// ------------------------------------
void ledG_toggle() {
  ledG.toggle();
  reportLedG_status();
}
// ===========================================================================================
// Red LED
// ------------------------------------
void reportLedR_status() {
  if (ledR.isOn()) {
    Serial.print("\nRed LED on");
  } 
  else {
    Serial.print("\nRed LED off");
  } 
}
// ------------------------------------
void ledR_toggle() {
  ledR.toggle();
  reportLedR_status();
}
// ===========================================================================================
// Yellow LED always blinking
// ------------------------------------
void blinkLedY() {
  static SDELAYMS_T ledDlyY(DUR_Y_OFF);  // Initially off and delay 1.8
  if (ledDlyY.dlyExpired()) {
    if (ledY.toggle()) {    // true if now on
      ledY.on();
      ledDlyY.dlySet(DUR_Y_ON);
      // Serial.print("\nY LED on");
    }
    else {
      ledY.off();
      ledDlyY.dlySet(DUR_Y_OFF);
    }
  }
}
