// dly-io.h
// Written by Wong SS
// Oct 2023
// Updated 2 Nov 2023 SOUTPUT toggle() now returns true if new value is on

// Define 3 classes SDELAYMS_T, SINPUT_T and SOUTPUT_T

// SDELAYMA_T: simple delay
// Modifed from original library (delays.h delays.cpp also  with timeout & timer) first created in 2014: 

// SINPUT_T: simple input
// Modifed from original library (input.h input.cpp) first created in 2013

// SOUTPUT_T: simple output
// Modifed from original library (output.h output.cpp) first created in 2017

#ifndef __DLY_IN_OUT_H__
#define __DLY_IN_OUT_H__

#include <Arduino.h>
// ==========================================================================================
class SDELAYMS_T {
  private:
    bool wasInit;
    uint32_t startTime;
    uint32_t desiredDly;
  public:
    SDELAYMS_T(uint32_t ms = 0);   // if 0, no delay set. set a delay later
    bool dlyChange(uint32_t ms);   // change desired dly if not expired yet; success return true
    void dlySet( uint32_t ms);     // set delay from now whether expired or not
    void dlySetSync( uint32_t ms); // set delay from end of last delay; if not expired, same as dlySet()
    bool dlyExpired( void );            // desired delay expired
    bool dlyExpiredRestart( void );     // expired, start a same delay from now
    bool dlyExpiredRestartSync( void ); // expired, start a same delay from end of last delay
    bool dlyTicked() {
      return dlyExpiredRestartSync();
    }
};
/*
  //Sample:
  SDELAYMS_T myDly(1000); //  delay object that will expire after 1000 ms
*/
// ==========================================================================================
#define IN_LOW      0  // no change and remains LOW
#define IN_HIGH     1 // no change and remains HIGH
#define IN_HIGH_LOW 2 // changed from HIGH to LOW
#define IN_LOW_HIGH 3 // changed from LOW to HIGH

class SINPUT_T {
  private:
    uint8_t pinNum;
    uint8_t pressedValue;
    uint8_t oldVal;
    uint16_t debounceDesired;
    uint32_t debounceStartTime;
    void debounceStart();
    bool debounceDone();
  public:
    // inputType = INPUT or INPUT_PULLUP
    SINPUT_T(uint8_t pin, uint8_t inputType, uint16_t bounceduration, uint8_t pv);
      // pin num, input type, bounce in ms, value when pressed/on
    uint8_t state( void );  // returns IN_LOW, IN_HIGH, IN_HIGH_LOW or IN_LOW_HIGH
    bool justPressed(); bool justOn();   // state() used
    bool justReleased(); bool justOff(); // state() used
    bool isPressed(); bool isOn();       // returns true if pin == pressedValue
    bool isReleased(); bool isOff();     // returns true if pin != pressedValue
    void changeBounceDur(uint16_t dur) {
      debounceDesired = dur;
    }
};
/*
  //Sample:
  SINPUT_T pb(2,INPUT_PULLUP,200,LOW); // pin 2 for input with pullup, 200ms bounce, LOW when pressed
*/
// ==========================================================================================
class SOUTPUT_T {
  private:
    bool isOn_flg;
    uint8_t pinNum;
    uint8_t onValue;
  public:
    SOUTPUT_T(uint8_t pNum, uint8_t onVal, bool initiallyOn = false);
    void on( void );     // output onValue to pin
    void off(void );     // output !onValue to pin
    bool toggle(void);   // returns true if on afer the toggle
    bool isOn( void );   // if currently on, return true
};
// --------------------------------------------------------------
/*
  // sample:
  SOUTPUT_T led(2,LOW); // pin 2 connected to the LED. LOW to on the LED
  // The constructor will set the pin to output
  // Default to off at the beginning

  // Want it on at the beginning:
  SOUTPUT_T led(2,LOW,true); // pin 2 connected to the LED. LOW to on the LED, on initially
*/
// ==========================================================================================
#endif
