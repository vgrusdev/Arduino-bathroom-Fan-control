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
 pin 11 - opto TRIAC control
 
*/

#include  <TimerOne.h>          // Avaiable from http://www.arduino.cc/playground/Code/Timer1

volatile int phase_counter=0;               // Variable to use as a counter volatile as it is in an interrupt
volatile int timer_decrease=0;
volatile boolean zero_cross=0;  // Boolean to store a "switch" to tell us if we have crossed zero

int AC_pin = 11;                // Output to Opto Triac

int rotation_speed = 0;                 // Rotation speed   0 = full, !=0 half
volatile int phase_skip=0;

int freqStep = 50;              // This is the delay-per-brightness step in microseconds.


void setup() {                                      // Begin setup
  phase_counter  = 0;
  phase_skip     = 0;
  timer_decrease = 0;
  freqStep       = 200;
  
  pinMode(AC_pin, OUTPUT);                          // Set the Triac pin as output
  pinMode(13, OUTPUT);
  attachInterrupt(0, zero_cross_detect, RISING);    // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection
  Timer1.initialize(freqStep);                      // Initialize TimerOne library for the freq we need
  Timer1.attachInterrupt(dim_check, freqStep);      
}

void zero_cross_detect() {   

  if (phase_counter++ >= phase_skip) {
    phase_counter = 0;
    delayMicroseconds(000);
    digitalWrite(13, HIGH); 
    digitalWrite(AC_pin, HIGH);
    delayMicroseconds(3000);
    digitalWrite(13, LOW); 
    digitalWrite(AC_pin, LOW);    
/*    
    timer_decrease = 1;
    zero_cross = true;
*/
  }
}                                 

// Turn on the TRIAC at the appropriate time
void dim_check() {
  if (zero_cross == true) {                   
    if (timer_decrease-- <= 0) {
      zero_cross = false;
/*
      digitalWrite(13, HIGH);
      digitalWrite(AC_pin, LOW);
*/
    }
  }                                 
}                                   

void loop() {  
/*  digitalWrite(13, HIGH);                      
*/
  delay(100);
}

