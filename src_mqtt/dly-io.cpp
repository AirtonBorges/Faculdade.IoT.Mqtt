// dly-io.cpp
// Written by Wong SS
// Oct 2023
// Updated 2 Nov 2023 SOUTPUT toggle() now returns true if new value is on

// Implement 3 classes: SDELAYMS_T, SINPUT_T, SOUTPUT_T
// See dly-io.h

#include "dly-io.h"
// ==========================================================================================
SDELAYMS_T::SDELAYMS_T( uint32_t ms ) {
  dlySet(ms);
}
// -------------------------------------------------
void SDELAYMS_T::dlySet( uint32_t ms ) {
  // allow to be set again even not expired yet
  // thus suitable for implementing timeout
  if (ms == 0) {
    wasInit = false;
    return;
  }
  startTime = millis(); // start from current time
  desiredDly = ms; // in ms
  wasInit = true;
}
// -------------------------------------------------
// This function prevents accumulative error
void SDELAYMS_T::dlySetSync( uint32_t ms ) {
  if (ms == 0) {
    wasInit = false;
    return;
  }
  if (wasInit) {
    // Not expired yet! Modify current delay!
    startTime = millis(); // start from current time
    desiredDly = ms;
  }
  else { // expired, new delay sync from end of last delay
    startTime = startTime + desiredDly; // start from old start + old desired delay
    desiredDly = ms;
  }
  wasInit = true;
}
// -------------------------------------------------
// if already expired, do nothing
bool SDELAYMS_T::dlyChange( uint32_t ms ) {
  if (ms == 0) {
    wasInit = false;
    return false;
  }
  if (wasInit) { // not expired yet
    // Not expired yet! Modify current delay!
    // Do not touch startTime
    desiredDly = ms;
    return true;
  }
  return false;
  // else do nothing
}
// -------------------------------------------------
bool SDELAYMS_T::dlyExpired( void ) {
  if (wasInit) {
    if ((millis() - startTime) >= desiredDly) {
      wasInit = false;
      return true; // expired
    }
    return false;
  }
  // not initialized, assume expired
  return true;
}
// -------------------------------------------------
bool SDELAYMS_T::dlyExpiredRestart( void ) {
  if (dlyExpired()) {
    dlySet(desiredDly);
    return true;
  }
  return false;
}
// -------------------------------------------------
bool SDELAYMS_T::dlyExpiredRestartSync( void ) {
  if (dlyExpired()) {
    dlySetSync(desiredDly);
    return true;
  }
  return false;
}
// ==========================================================================================
SINPUT_T::SINPUT_T(uint8_t pin, uint8_t inputType, uint16_t bounceduration, uint8_t pv) {
  if (inputType != INPUT) {
    inputType = INPUT_PULLUP; // if INPUT_PULLUP or wrong value
  }
  if (pv) {
    pressedValue = HIGH;
  }
  else {
    pressedValue = LOW;
  }
  pinMode(pin, inputType);
  pinNum = pin;
  oldVal = digitalRead(pin); // take current value as old value
  debounceDesired = bounceduration;
  debounceStart(); // assuming debounces at the beginning
}
// -------------------------------------------------
uint8_t SINPUT_T::state( void ) {
  if (!debounceDone()) return oldVal;
  if (digitalRead(pinNum) != oldVal) {
    // a change
    debounceStart();
    oldVal = !oldVal;
    if (oldVal == 0) {
      return IN_HIGH_LOW;
    }
    else {
      return IN_LOW_HIGH;
    }
  }
  else {
    // no change
    return oldVal;
  }
}
// -------------------------------------------------
void SINPUT_T::debounceStart() {
  debounceStartTime = millis();
}
// -------------------------------------------------
bool SINPUT_T::debounceDone() {
  if ((millis() - debounceStartTime) >= debounceDesired) {
    return true;
  }
  return false;
}
// -------------------------------------------------
// Determine if the input is "just pressed" or "just On", i.e. change from "released" to "pressed".
// If the pressedOnValue was not set to HIGH or LOW, it always return false.
// IMPORTANT: Beware that internally this method uses state(). Read CAUTION section below.
// if (pb.justPressed()) {    // or pb.justOn()
//   ...   // pb just pressed
// }
bool SINPUT_T::justPressed() {
  switch (state()) {
    case IN_HIGH_LOW:
      if (pressedValue == LOW) {
        return true;
      }
      break;
    case IN_LOW_HIGH:
      if (pressedValue == HIGH) {
        return true;
      }
  }
  return false;
}
// -------------------------------------------------
bool SINPUT_T::justOn() {
  return justPressed();
}
// -------------------------------------------------
// Determine if the input is "just pressed", i.e. change from "released" to "pressed".
// If the pressedOnValue was not set to HIGH or LOW, it always return false.
// IMPORTANT: Beware that internally this method uses state(). Read CAUTION section below.
bool SINPUT_T::justReleased() {
  switch (state()) {
    case IN_HIGH_LOW:
      if (pressedValue == HIGH) return true;
      break;
    case IN_LOW_HIGH:
      if (pressedValue == LOW) return true;
  }
  return false;
}
bool SINPUT_T::justOff() {
  return justReleased();
}
// -------------------------------------------------
// Determine if the input is currently "pressed" or "on".
// If the pressedOnValue was not set to HIGH or LOW, it always return false.
bool SINPUT_T::isPressed() {
  if (digitalRead(pinNum) == pressedValue) {
    return true;
  }
  return false;
}
// -------------------------------------------------
bool SINPUT_T::isOn() {
  return isPressed();
}
// ==========================================================================================
SOUTPUT_T::SOUTPUT_T(uint8_t pNum, uint8_t onVal, bool initiallyOn) {
  pinNum = pNum;
  onValue = onVal ? 1 : 0; // either 1 or 0 only
  if (initiallyOn) {
    on();
  }
  else {
    off();
  }
  // set mode only after "output" value
  pinMode(pNum, OUTPUT);
}
// -------------------------------------------------
void SOUTPUT_T::on( void ) {
  digitalWrite(pinNum, onValue);
  isOn_flg = true;
}
// -------------------------------------------------
void SOUTPUT_T::off(void ) {
  digitalWrite(pinNum, !onValue);
  isOn_flg = false;
}
// -------------------------------------------------
bool SOUTPUT_T::toggle(void) {
  if (isOn_flg == false) {
    on();
    return true;
  }
  else {
    off();
    return false;
  }
}
// -------------------------------------------------
bool SOUTPUT_T::isOn ( void ) {
  return isOn_flg;
}
// ==========================================================================================
