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
#include <DHT.h>

#define DEB 1
//#define DEB1 1

#define ZXD_pin  2
#define FAN_pin 13

#define DHTPIN 8
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define ON  true
#define OFF false

volatile unsigned long zero_cross_time = (unsigned long)0;
volatile bool          zero_crossed;

bool prev_switch_status;


byte FAN_status=(byte)0;
//	0	-  Initial state. FAN stopped. Move out only if SWITCH turned ON
//	1	-  SWITCH turned ON, FAN start delayed.
//  2	-  SWITCH turned ON, FAN turned ON
//	3	-  SWITCH turned OFF, FAN turned ON, FAN stop delayed.
//
//

boolean cur_fan = OFF;
unsigned long FAN_status_time;
unsigned long FAN_start_time;
unsigned long FAN_stop_time;

const unsigned long FAN_start_delay_day		= (unsigned long)1 * 6 * 1000;
const unsigned long FAN_start_delay_night	= (unsigned long)3 * 6 * 1000;
const unsigned long FAN_stop_delay_day		= (unsigned long)3 * 6 * 1000;
const unsigned long FAN_stop_delay_night	= (unsigned long)1 * 6 * 1000;

const unsigned long FAN_running_time      = (unsigned long)4 * 6 * 1000;
const unsigned long FAN_pause_time        = (unsigned long)1 * 6 * 1000;
      	       int  FAN_pause_cycles	  	= 0;

const int Cycles_day		  		= 4;
const int Cycles_night				= 1;

const float HUMIDITY_high_level = 82.0;
const float HUMIDITY_low_level  = 76.0;
       bool HUMIDITY_flag = true;

void setup() {                                      // Begin setup
  
  Wire.begin();
  dht.begin();

  pinMode(ZXD_pin, INPUT_PULLUP);
  pinMode(FAN_pin, OUTPUT);

  zero_cross_time = millis();
  zero_crossed    = false;
  attachInterrupt(0, zero_cross_detect, RISING);    // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection


  Serial.begin(115200);
#ifdef DEB
  Serial.println("Fan control");  

  Serial.print("FAN_running_time = ");
  Serial.println(FAN_running_time);
  Serial.print("FAN_pause_time = ");
  Serial.println(FAN_pause_time);
#endif
  

  FAN_status_time = millis();
  FAN_start_time  = FAN_status_time;
  FAN_stop_time   = FAN_status_time;
  FAN_turn( ON );
  delay(100);
  FAN_turn( OFF );

  prev_switch_status = OFF;
  if (is_switch_on()) {
    FAN_status = 1;
  }
}

void zero_cross_detect() {   
  zero_cross_time = millis();
  zero_crossed = true;
}                                 

bool is_switch_on() {

  unsigned long td;
  unsigned long zc;
  unsigned long cur_time = millis();

  noInterrupts();
    zc = zero_cross_time;
  interrupts();
  td = timeDiff(zc, cur_time);
  if (td > 50) {
    return OFF;
  } else {
#ifdef DEB1
    Serial.print("****** ===>  Returning switch ON. zero_cross_time = ");
    Serial.print(zc);
    Serial.print(". cur_time = ");
    Serial.print(cur_time);
    Serial.print(". Diff = ");
    Serial.println(td);
    Serial.print("ON = ");
    Serial.println(ON);
#endif
    return ON;
  }
}

bool is_switch_on_v() {
  bool cur_status, last_status;
  int count = 0;

  last_status = is_switch_on();
  while(count < 3) {
    delay(15);
    cur_status = is_switch_on();
    if (cur_status == last_status) { ++count; }
    else {
      last_status = cur_status;
      count = 0;
    }
  }
  return cur_status;
}
bool is_day_now() {

  static DS3231 ds3231;

  bool h12, pm = false;
  byte h = ds3231.getHour(h12, pm);

  if ((h > 8) && (h < 23)) {
    return true;
  } else {
    return false;
  }
}

void FAN_turn(bool new_fan) {

//  static boolean cur_fan = OFF;

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

void FAN_pause_check() {

  unsigned long cur_millis = millis();
  int cycles;

  if (cur_fan == ON) {	                                  	// FAN if ON
    if (timeDiff(FAN_start_time, cur_millis) > FAN_running_time) {
      ++FAN_pause_cycles;
      FAN_turn( OFF );
#ifdef DEB
      Serial.println("FAN paused.");
#endif
    }
  } else {					// FAN is OFF
    if ( is_day_now() )  { cycles = Cycles_day; }
    else                 { cycles = Cycles_night; }
    if ((FAN_pause_cycles < cycles) && 
         timeDiff(FAN_stop_time, cur_millis) > FAN_pause_time) {
      FAN_turn( ON );
#ifdef DEB
      Serial.println("FAN resumed.");
#endif
    } 
  }
}

bool FAN_pause_check_b() {

  unsigned long cur_millis = millis();
  int cycles;

  bool stuck_ret = false;

  if (cur_fan == ON) {                                      // FAN if ON
    if (timeDiff(FAN_start_time, cur_millis) > FAN_running_time) {
      ++FAN_pause_cycles;
      FAN_turn( OFF );
#ifdef DEB
      Serial.println("FAN paused.");
#endif
    }
  } else {          // FAN is OFF
    if ( is_day_now() )  { cycles = Cycles_day; }
    else                 { cycles = Cycles_night; }
    if (FAN_pause_cycles < cycles) {
      if (timeDiff(FAN_stop_time, cur_millis) > FAN_pause_time) {
        FAN_turn( ON );
#ifdef DEB
        Serial.println("FAN resumed.");
#endif
      }
    } else {
#ifdef DEB
      Serial.println("FAN pause cycles over.");
#endif
      stuck_ret = true;
    }
  }
  return stuck_ret;
}


float Humidity() {

  static float h_prev = 30.0;
  static unsigned long prev_millis = 0;

  unsigned long cur_millis = millis();
// --->
//  h_prev = dht.readHumidity();
//  interrupts();
//  delay(100);
//  return h_prev;
// <---

  if (timeDiff(prev_millis, cur_millis) >= 5000) {

    prev_millis = cur_millis;
    h_prev = dht.readHumidity(false);
//    interrupts();
//    delay(20);
    if (isnan(h_prev)) { h_prev = 30.0; }
  }
  return h_prev;
}

unsigned long timeDiff (unsigned long start, unsigned long stop) {

  unsigned long t;

  if (start > stop) {
    t = (unsigned long)4294967295 - (start - stop);
  } else {
    t = stop - start;
  }
  return t;
}


// ===================================================
//  Start Loop here
//====================================================
void loop() {  

#ifdef DEB1

  if (zero_crossed) {
    zero_crossed = false;
    Serial.print("***** ==>  Zero crossed. time: ");
    noInterrupts();
    unsigned long zxt = zero_cross_time;
    interrupts();
    Serial.println(zxt);
  }
#endif

  unsigned long timediff;

  switch (FAN_status) {

    case 0:
//  Initial state. FAN stopped. Move out only if SWITCH turned from OFF -> to ON
//
      if (is_switch_on()) {
        if ( ! prev_switch_status ) {
          HUMIDITY_flag = true;
          FAN_pause_cycles = 0;
          FAN_status = 1;
	        FAN_status_time = millis();
#ifdef DEB
          Serial.println("Go to FAN status 1.");
#endif

        } else {
          //block here, do not use !!
        }
      } else {
        prev_switch_status = false;
//
        if (Humidity() > HUMIDITY_high_level) {
#ifdef DEB
          Serial.print("Humidity higher than limit: ");
          Serial.println(Humidity());
#endif
          if (HUMIDITY_flag) {
            HUMIDITY_flag = false;
//            FAN_pause_check();
            FAN_status = 5;
//            FAN_pause_cycles = 0;
            FAN_status_time = millis();          
#ifdef DEB
            Serial.println("Go to FAN status 5.");
#endif
          }
        }
      }
      break;
    case 1:
//  SWITCH turned ON, FAN start delayed.
//
      if ( is_switch_on()) {
        if ( is_day_now() )  { timediff = FAN_start_delay_day; }
        else		             { timediff = FAN_start_delay_night; }
        if ( timeDiff(FAN_status_time, millis()) >= timediff ) {
	        FAN_status = 2;
		      FAN_pause_cycles = 0;
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
        FAN_pause_check();
      } else {
        FAN_pause_cycles = 0;
        if (cur_fan == ON) {         // FAN if ON
          FAN_status = 3;
          FAN_status_time = millis();
#ifdef DEB
          Serial.println("Go to FAN status 3.");
#endif
        } else {					// FAN is OFF
          prev_switch_status = is_switch_on();
          FAN_status = 0;
          FAN_status_time = millis();
          FAN_turn( OFF );
#ifdef DEB
          Serial.println("Go to FAN status 0.");
#endif
        }
      }
      break;
    case 3:
//  SWITCH turned OFF, FAN turned ON, FAN stop delayed.
//
      if ( is_day_now() ) { timediff = FAN_stop_delay_day; }
      else                { timediff = FAN_stop_delay_night; }
      if ( timeDiff(FAN_status_time, millis()) >= timediff ) {
        prev_switch_status = is_switch_on();
        FAN_status = 0;
        FAN_status_time = millis();
        FAN_turn( OFF );
#ifdef DEB
        Serial.println("Go to FAN status 0.");
#endif
      }
      break;
      
    case 5:

      if ( is_switch_on_v() ) {
#ifdef DEB
        Serial.println("Switch turned ON");
#endif
        HUMIDITY_flag = true;
        FAN_pause_cycles = 0;
        if (cur_fan == ON) {         // FAN if ON
          FAN_status = 2;
          FAN_status_time = millis();
//          FAN_turn( ON );
#ifdef DEB
          Serial.println("Go to FAN status 2.");
#endif
        } else {                     // FAN is OFF
          FAN_status = 1;
          FAN_status_time = millis();
#ifdef DEB
          Serial.println("Go to FAN status 1.");
#endif          
        }
      } else { 
        if (Humidity() < HUMIDITY_low_level) {
#ifdef DEB
          Serial.print("Humidity is below high level: ");
          Serial.println(Humidity());
#endif
          FAN_pause_cycles = 0;
          prev_switch_status = is_switch_on();
          FAN_status = 0;
          FAN_status_time = millis();
          FAN_turn( OFF );
#ifdef DEB
          Serial.println("Go to FAN status 0.");
#endif
        } else {
          FAN_pause_check();
        }
      }
      break;
      
      default:
        {}
#ifdef DEB1
      Serial.println("Error - Unknown FAN_status.");
#endif
//
  }

  delay(100);

}

