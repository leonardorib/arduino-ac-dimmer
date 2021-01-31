/*  
      Controls Power in AC based on user Serial Input (powerLevel from 1 to 100) using a AC dimmer module  
    with zero-cross detection.

    Author: Leonardo Batista Ribeiro.
*/

// LIBRARIES
#include <TimerOne.h>

// PIN DEFINITIONS
#define TRIAC_PIN 12
#define ZERO_CROSS_PIN 2

// Definines power level varying from 1 to 100 and initializes it with 10
unsigned int maxPowerLevel = 100;
unsigned int minPowerLevel = 1;
unsigned int powerLevel = 10;

// Time in microseconds to trigger the TRIAC and allow current to the load. Longer time -> Less power
// Half AC cycle duration in 60hz: (1/60)/2 = 8333 microseconds.
unsigned int maxTimeTRIAC = 8000;                   // Equivalent to MIN PowerLevel
unsigned int minTimeTRIAC = 1000;                   // Equivalent to MAX PowerLevel

// Initializes the timeTRIAC variable by mapping the powerLevel variable, taking in to account the inverse relation
unsigned int timeTRIAC = map(powerLevel, minPowerLevel, maxPowerLevel, maxTimeTRIAC, minTimeTRIAC);

// TIMER1 INTERRUPT ROUTINE - Triggered when timeTRIAC passed (Timer Overflow)
void  timerInterrupt() {
  // Stops Timer1
  Timer1.stop();  

  // Triggers the TRIAC by sending a pulse of 10 microseconds
  digitalWrite(TRIAC_PIN, HIGH);                    
  delayMicroseconds(10);
  digitalWrite(TRIAC_PIN, LOW);                              
}

// ZERO_CROSS_INTERRUPT - Triggered when the ZeroCross pin receives a pulse (current is zero)
void  zeroCrossInterrupt() {
  // Restarts Timer1
  Timer1.resume();                                  
}

// SETUP - Executed once on board initialization
void setup() {
  Serial.begin(9600);

  pinMode(TRIAC_PIN, OUTPUT);
  pinMode(ZERO_CROSS_PIN, INPUT);

  // Initializes Timer1 with the initial time previously defined
  Timer1.initialize(timeTRIAC);

  // Attaches the timerInterrupt function to the Timer1 to be triggered when Timer1 overflows
  Timer1.attachInterrupt(timerInterrupt);           

  // Attaches the zeroCrossInterrupt function to be triggered when 0 (digital 2 on Arduino UNO) receives a pulse
  attachInterrupt(0, zeroCrossInterrupt, RISING);

  Serial.println("Select a Power Level between 1 and 100%");
}

// LOOP - Continuously executed
void loop() {
  // Gets user input through Arduino IDE serial monitor
  if (Serial.available() > 0) {
    int integerOnSerialBuffer = Serial.parseInt(); // If no number is provided, returns 0.

    // Checks if it's different than 0. (Meaning that a number was actually provided)
    if (integerOnSerialBuffer != 0) {
      powerLevel = integerOnSerialBuffer;

      // Limits user input possibilities
      if (powerLevel > maxPowerLevel) {
        powerLevel = maxPowerLevel;
      };
      if (powerLevel < minPowerLevel) {
        powerLevel = minPowerLevel;
      };

      // Converts the power level selected to the time before triggering the TRIAC
      timeTRIAC = map(powerLevel, minPowerLevel, maxPowerLevel, maxTimeTRIAC, minTimeTRIAC);
      Timer1.setPeriod(timeTRIAC);

      Serial.print("Power Level is now: ");Serial.print(powerLevel);Serial.println("%.");
    }
    delay(50);
  }

  delay(50);
}
