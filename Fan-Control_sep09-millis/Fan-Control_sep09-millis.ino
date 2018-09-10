/*AC Fan Control

  AC Voltage dimmer with Zero cross detection

  Attach the Zero cross pin of the module to Arduino External Interrupt pin
  Select the correct Interrupt # from the below table 
  (the Pin numbers are digital pins, NOT physical pins: 
  digital pin 2 [INT0]=physical pin 4 and digital pin 3 [INT1]= physical pin 5)
  check: <a href="http://arduino.cc/en/Reference/attachInterrupt">interrupts</a>

  Pin    |  Interrrupt # | Arduino Platform
  ---------------------------------------
  2      |  0            |  All -But it is INT1 on the Leonardo
  3      |  1            |  All -But it is INT0 on the Leonardo
  18     |  5            |  Arduino Mega Only
  19     |  4            |  Arduino Mega Only
  20     |  3            |  Arduino Mega Only
  21     |  2            |  Arduino Mega Only
  0      |  0            |  Leonardo
  1      |  3            |  Leonardo
  7      |  4            |  Leonardo
The Arduino Due has no standard interrupt pins as an iterrupt can be attached to almosty any pin. 

 pin 2  - Zero cross detection
 
*/

#include <Wire.h>
#include "DS3231.h"

#define DEB 1

#define ZXD_pin  2
#define FAN_pin 13

#define ON  true
#define OFF false

volatile unsigned long zero_cross_time;

bool prev_switch_status;


byte FAN_status=(byte)0;
//	0	-  Initial state. FAN stopped. Move out only if SWITCH turned ON
//	1	-  SWITCH turned ON, FAN start delayed.
//      2	-  SWITCH turned ON, FAN turned ON
//	3	-  SWITCH turned OFF, FAN turned ON, FAN stop delayed.
//
//

unsigned long FAN_status_time;
unsigned long FAN_start_time;
unsigned long FAN_stop_time;

const unsigned long FAN_start_delay_day		= 1 * 6 * 1000;
const unsigned long FAN_start_delay_night	= 3 * 6 * 1000;
const unsigned long FAN_stop_delay_day		= 3 * 6 * 1000;
const unsigned long FAN_stop_delay_night	= 1 * 6 * 1000;


void setup() {                                      // Begin setup
  
  Wire.begin();

  pinMode(FAN_pin, OUTPUT);
  attachInterrupt(0, zero_cross_detect, RISING);    // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection


  Serial.begin(115200);
#ifdef DEB
  Serial.println("Fan control");  
#endif

  prev_switch_status = OFF;
  if (is_switch_on()) {
    FAN_status = 1;
  }
  FAN_status_time = millis();
  FAN_start_time  = FAN_status_time;
  FAN_stop_time   = FAN_status_time;
  FAN_turn( ON );
  delay(100);
  FAN_turn( OFF );
}

void zero_cross_detect() {   
  zero_cross_time = millis();
}                                 

bool is_switch_on() {

  unsigned long now = millis();

  noInterrupts();
    unsigned long timediff = now - zero_cross_time;
  interrupts();
  if (timediff > 60) {
    return OFF;
  } else {
    return ON;
  }
}

bool is_day_now() {

  DS3231 ds3231;

  bool h12, pm = false;
  byte h = ds3231.getHour(h12, pm);

  if ((h > 8) && (h < 23)) {
    return true;
  } else {
    return false;
  }
}

void FAN_turn(bool new_fan) {

  static boolean cur_fan = OFF;

  if ( new_fan != cur_fan) {
    cur_fan = new_fan;
    if ( new_fan ) { 
      FAN_start_time = millis();
      digitalWrite(FAN_pin, HIGH); 
#ifdef DEB
      Serial.println("FAN turned ON.");
#endif
    } else { 
      FAN_stop_time = millis();
      digitalWrite(FAN_pin, LOW);
#ifdef DEB
      Serial.println("FAN turned OFF.");
#endif

    }
  }
}


// ===================================================
//  Start Loop here
//====================================================
void loop() {  

  unsigned long timediff;

  switch (FAN_status) {

    case 0:
//  Initial state. FAN stopped. Move out only if SWITCH turned from OFF -> to ON
//
      if (is_switch_on()) {
        if ( ! prev_switch_status ) {
          FAN_status = 1;
	  FAN_status_time = millis();
#ifdef DEB
          Serial.println("Go to FAN status 1.");
#endif

        } else {
        }
      } else {
        prev_switch_status = false;
      }
      break;
    case 1:
//  SWITCH turned ON, FAN start delayed.
//
      if ( is_switch_on()) {
        if ( is_day_now() )  { timediff = FAN_start_delay_day; }
        else		             { timediff = FAN_start_delay_night; }
        if ( (millis() - FAN_status_time) >= timediff ) {
	        FAN_status = 2;
	        FAN_status_time = millis();
	        FAN_turn( ON );
#ifdef DEB
          Serial.println("Go to FAN status 2.");
#endif
        }
      } else {
        prev_switch_status = false;
        FAN_status = 0;
        FAN_status_time = millis(); 
#ifdef DEB
        Serial.println("Go to FAN status 0.");
#endif
      }

      break;
    case 2:
//  SWITCH turned ON, FAN turned ON
//
      if ( is_switch_on()) { 
// FAN_pause_check()
        FAN_status = FAN_status;
      } else {
        FAN_status = 3;
        FAN_status_time = millis();
        FAN_turn( ON );
#ifdef DEB
        Serial.println("Go to FAN status 3.");
#endif
      }
      break;
    case 3:
//  SWITCH turned OFF, FAN turned ON, FAN stop delayed.
//
      if ( is_day_now() ) { timediff = FAN_stop_delay_day; }
      else                { timediff = FAN_stop_delay_night; }
      if ( (millis() - FAN_status_time) >= timediff ) {
        prev_switch_status = is_switch_on();
        FAN_status = 0;
        FAN_status_time = millis();
        FAN_turn( OFF );
#ifdef DEB
        Serial.println("Go to FAN status 0.");
#endif
      }
      break;
//    default:
//
  }

  delay(100);

}

