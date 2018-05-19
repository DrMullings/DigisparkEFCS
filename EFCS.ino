#include "Arduino.h"
#include "Bounce2-master\Bounce2.h"
#include "avr\interrupt.h"
#include "avr\sleep.h"

/*******************************************************************************
Settings
*******************************************************************************/

//#define IS_AN94	1			// Is EFCS used for Abakan
//#define IS_ATTINY 1			// Use for Digispark
//#define IS_SEMI_BURST 1		// Use for Semi/Burst Mode
//#define ENABLE_BURST 1		// Uncomment to enable burst fire
//#define ENABLE_FULLAUTO 1		// Uncomment to enable full auto
//#define DEBUG 1				// Debug Mode

#ifdef IS_AN94
#define BURST_CNT 2			// DO NOT TOUCH
#else
#define BURST_CNT 3			// Set Burst here
#endif

/*******************************************************************************
Pins
*******************************************************************************/

// do not change!
#define PIN_TRG 2			// Trigger

// set according your setup
#define PIN_COL 3			// COL
#define PIN_SEM 5			// Semi
#define PIN_FET 4			// MosFet Gate
#define PIN_BRT 6			// Burst
#define PIN_FLA 7			// Full Auto

#ifndef IS_ATTINY
#define PIN_BLT 9			// Bolt Stop
#define PIN_MSG 13			// Message Pin default onboard LED
#endif // !IS_ATTINY



/*******************************************************************************
Global constants
*******************************************************************************/

// only touch when knowing what to do

#define DEB_TRG 50			// debounce time for trigger
#define DEB_COL 2			// debounce time for COL
#define MAX_CYC 500			// maximum cycle time default 50

#ifdef IS_AN94
#define RPM_LIM 600		//do not edit
#else
#define RPM_LIM	0			// RPM Limit - 0=unlimited - edit here
#endif

Bounce colBouncer = Bounce();

/*******************************************************************************
Global variables
*******************************************************************************/
//do not touch
bool triggerPressed = false;
int cycleLength = 0;
int rpmDelay = 0;
long lastTrigger = -1;
int errorCnt = 0;

/*******************************************************************************
Setup
*******************************************************************************/


void setup() {

#ifdef DEBUG
	Serial.begin(115200);
	Serial.println("Setting up pins");
#endif // DEBUG

	lastTrigger = millis();
	triggerPressed = false;

	//Pin setup
	pinMode(PIN_TRG, INPUT_PULLUP);
	pinMode(PIN_COL, INPUT_PULLUP);
	pinMode(PIN_BRT, INPUT_PULLUP);
	pinMode(PIN_FLA, INPUT_PULLUP);
	pinMode(PIN_FET, OUTPUT);
	pinMode(PIN_MSG, OUTPUT);

	// if we had an error
	if (errorCnt > 0) {
#ifdef DEBUG
		Serial.println("An error occured");
#endif // DEBUG
		digitalWrite(PIN_MSG, HIGH);
		errorCnt = 0;
	}

	colBouncer.attach(PIN_COL);
	colBouncer.interval(DEB_COL);

	//initialize Pins with LOW
	digitalWrite(PIN_FET, LOW);

	attachInterrupt(0, isr_fire, FALLING);

	// calculate RPM
	// 60000ms(=1min) / RPM
	if (RPM_LIM > 0) {
		rpmDelay = 60000 / RPM_LIM;
	}
	
#ifdef DEBUG
	Serial.print("rpmDelay: ");
	Serial.println(rpmDelay);
	Serial.print("RPM Limit: ");
	Serial.println(RPM_LIM);
	
	// Test Blink
	digitalWrite(PIN_MSG, HIGH);
	delay(1000);
	digitalWrite(PIN_MSG, LOW);

	Serial.println("Setup done \n");
#endif // DEBUG
	
}

/*******************************************************************************
Loop
*******************************************************************************/


void loop() {
	if (triggerPressed) {
		detachInterrupt(0);
#ifdef DEBUG
		Serial.println("Trigger has been pressed");
#endif // DEBUG

// SEMI MODE
		if (is_semi()) {
#ifdef DEBUG
			Serial.println("Semi mode");
#endif // DEBUG
			cycle();
#ifdef SEMI_BURST
			if (digitalRead(PIN_TRG) == LOW) {
				for (int i = 0; i < BURST_CNT; i++) {
					cycle();
				}
			}
#endif // SEMI_BURST
		}

// BURST MODE
#ifdef ENABLE_BURST
		else if (is_burst()) {
#ifdef DEBUG
			Serial.println("Burst mode");
#endif // DEBUG
			for (int i = 0; i < BURST_CNT; i++) {
				cycle();
				if (RPM_LIM > 0) {
					delay(rpmDelay - cycleLength);
				}
			}
		}
#endif

// FULL AUTO MODE
#ifdef ENABLE_FULLAUTO
		else if (is_full()) {
#ifdef DEBUG
			Serial.println("Full auto mode");
#endif // DEBUG
#ifdef IS_AN94
			cycle();
			cycle();
#endif
			while (digitalRead(PIN_TRG)) {
				cycle();
				if (RPM_LIM > 0) {
					delay(rpmDelay - cycleLength);
				}
			}
		}
#endif

// FUCKUP
		else {
			// we should never get here
			// re-initalize everything
#ifdef DEBUG
			Serial.println("Error in loop(), trigger pressed without fire mode");
#endif // DEBUG
			errorCnt++;
			setup();
		}
		// reset trigger
		attachInterrupt(0, isr_fire, FALLING);
		triggerPressed = false;		
	}

	else if (is_safe()) {
		// energy saving here
		// maybe sleep?
		
		//enable pin change interrupt on VECT2 0-8

	}

	else {
		// trigger not pressed for longer time?
		// maybe sleep?
	}
}

/*******************************************************************************
Functions
*******************************************************************************/


inline void cycle() {
	int startCycle = millis();
#ifdef DEBUG
	Serial.println("Starting to cycle");
	Serial.print("startCycle: ");
	Serial.println(startCycle);
#endif // DEBUG

	digitalWrite(PIN_FET, HIGH);

	do {
		colBouncer.update();
		// limiting the maximum cycle time
		if (millis() - startCycle > MAX_CYC) {
#ifdef DEBUG
			Serial.print("Cycling too long: ");
			Serial.print(millis()-startCycle);
			Serial.println(" milliseconds");
#endif // DEBUG
			//error handling here
			//re-initialize everything
			errorCnt++;
			setup();
			break;
		}
	} while (!colBouncer.rose());

	digitalWrite(PIN_FET, LOW);

	int endCycle = millis();
	cycleLength = endCycle - startCycle;
#ifdef DEBUG
	Serial.print("Cycle ends, took ");
	Serial.print(cycleLength);
	Serial.println(" milliseconds");
#endif // DEBUG
}

void isr_fire() { 

#ifdef DEBUG
	Serial.println("Interrupt caught");
#endif // DEBUG

	int currTrigger = millis();
	// debouncing
	if (currTrigger - lastTrigger > DEB_TRG) {
		triggerPressed = true;
	}
	lastTrigger = currTrigger;
}

inline volatile bool is_safe() { return !(digitalRead(PIN_SEM) && digitalRead(PIN_BRT) && digitalRead(PIN_FLA)); }

inline volatile bool is_semi() { return !(digitalRead(PIN_SEM)); }

#ifdef ENABLE_BURST
inline volatile bool is_burst() { return !digitalRead(PIN_BRT); }
#endif // ENABLE_BURST

#ifdef ENABLE_FULLAUTO
inline volatile bool is_full() { return !digitalRead(PIN_FLA); }
#endif // ENABLE_FULLAUTO


