/*  
      Controls Power in AC based on user Serial Input (powerLevel from 1 to 100) using a AC dimmer module  
    with zero-cross detection while monitors temperature with a DS18B20 simultaneously using non-blocking 
    code, avoiding flickering in the AC load.

    Author: Leonardo Batista Ribeiro.
*/

// LIBRARIES
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimerOne.h>

// PIN DEFINITIONS
#define TRIAC_PIN 12
#define ZERO_CROSS_PIN 2
#define DS18B20_PIN 8                               // DS18B20 temperature sensor data pin

// INITIALIZES DS18B20 LIBRARIES
OneWire oneWire(DS18B20_PIN);                       // Setup a oneWire instance
DallasTemperature temperatureSensor(&oneWire);      // Pass the oneWire reference to Dallas Temperature.

// VARIABLES DEFINITIONS
float temperature;

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

// DS18B20 Resolution (see sensor datasheet)
unsigned int temperatureResolution = 12;

// Time necessary to get the temperature according to the resolution 
// See sensor datasheet and WaitForConversionExample in the DallasTemperature library
unsigned long  tempMeasureDelay = 750 / (1 << (12 - temperatureResolution));

// Auxiliar variables to count time
unsigned long lastTemperatureRequestTime = 0;
unsigned long temperatureRequestInterval = 2000;
boolean temperatureWasRequested = false;
unsigned long currentTime = 0;

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

  // DS18B20 Temperature Sensor Setup
  temperatureSensor.begin();
  temperatureSensor.setResolution(temperatureResolution);
  temperatureSensor.setWaitForConversion(false);            // Setting to false to measure in a non-blocking way
  temperatureSensor.requestTemperatures();                  // First temperature request
  lastTemperatureRequestTime = millis();                    // Register the time when the temperature was requested
  temperatureWasRequested = true;                           // Indicates that a request was made
}

// LOOP - Continuously executed
void loop() {
  // Gets the temperature value and print if the time necessary to temperature conversion has passed since the last temperature request
  currentTime = millis();
  if ((currentTime - lastTemperatureRequestTime) >= tempMeasureDelay && temperatureWasRequested == true ) {

    // Gets the temperature value after checking if the conversion in the sensor was actually completed
    if (temperatureSensor.isConversionComplete() == true) {

      temperature = temperatureSensor.getTempCByIndex(0);

      Serial.println(temperature);
      temperatureWasRequested = false;
    }
  } 

  // Makes a temperature request to the sensor if the interval defined has passed since the last temperature request
  currentTime = millis();
  if ((currentTime - lastTemperatureRequestTime) >= temperatureRequestInterval && temperatureWasRequested == false ) {
    temperatureSensor.requestTemperatures();
    lastTemperatureRequestTime = millis();
    temperatureWasRequested = true;
  }

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
    }
    delay(50);
  }

  delay(50);
}
