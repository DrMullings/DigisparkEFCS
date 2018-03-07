#include "Arduino.h"
#include "Bounce2-master\Bounce2.h"

/*******************************************************************************
Settings
*******************************************************************************/

#define IS_AN94	1			// Is EFCS used for Abakan

#ifdef IS_AN94
#define BURST_CNT 2			// DO NOT TOUCH
#else
#define BURST_CNT 3			// Set Burst here
#endif

/*******************************************************************************
Pins
*******************************************************************************/

// do not change!
#define PIN_TRG 7			// Trigger

// set according your setup
#define PIN_COL 1			// COL
#define PIN_SEM 7			// Semi
#define PIN_FET 3			// MosFet Gate
#define PIN_BRT 4			// Burst
#define PIN_FLA 5			// Full Auto
#define PIN_BLT 6			// Bolt Stop
#define PIN_MSG 8			// Message Pin buzzer or LED

/*******************************************************************************
Global constants
*******************************************************************************/

// only touch when knowing what to do

#define DEB_TRG 100			// debounce time for trigger
#define DEB_COL 2			// debounce time for COL
#define MAX_CYC 5000		// maximum cycle time

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


	//Pin setup
	pinMode(PIN_TRG, INPUT_PULLUP);
	pinMode(PIN_COL, INPUT_PULLUP);
	pinMode(PIN_BRT, INPUT_PULLUP);
	pinMode(PIN_FLA, INPUT_PULLUP);
	pinMode(PIN_FET, OUTPUT);
	pinMode(PIN_MSG, OUTPUT);

	// if we had an error
	if (errorCnt > 0) {
		digitalWrite(PIN_MSG, HIGH);
		errorCnt = 0;
	}

	colBouncer.attach(PIN_COL);
	colBouncer.interval(DEB_COL);

	//initialize Pins with LOW
	digitalWrite(PIN_FET, LOW);

	attachInterrupt(0, isr, RISING);

	// calculate RPM
	// 60000ms(=1min) / RPM
	if (RPM_LIM > 0) {
		rpmDelay = 60000 / RPM_LIM;
	}
}

/*******************************************************************************
Loop
*******************************************************************************/


void loop() {
	if (triggerPressed) {

		if (is_semi()) {
			cycle();
		}
		else if (is_burst()) {
			for (int i = 0; i < BURST_CNT; i++) {
				cycle();
				if (RPM_LIM > 0) {
					delay(rpmDelay - cycleLength);
				}
			}
		}
		else if (is_full()) {
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
		else {
			// we should never get here
			// re-initalize everything
			errorCnt++;
			setup();
		}
		// reset trigger
		triggerPressed = false;
	}
}

/*******************************************************************************
Functions
*******************************************************************************/


inline void cycle() {
	int startCycle = millis();
	digitalWrite(PIN_FET, HIGH);
	while (colBouncer.update() && colBouncer.fell()) {

		// limiting the maximum cycle time
		if (millis() - startCycle > MAX_CYC) {
			//error handling here
			//re-initialize everything
			errorCnt++;
			setup();
			break;
		}
	}
	digitalWrite(PIN_FET, LOW);
	int endCycle = millis();
	cycleLength = endCycle - startCycle;
}

void isr() { 
	int currTrigger = millis();
	// debouncing
	if (currTrigger - lastTrigger < DEB_TRG) {
		triggerPressed = true;
	}
	lastTrigger = currTrigger;
}

inline bool is_semi() { return digitalRead(PIN_SEM); }
inline bool is_burst() { return digitalRead(PIN_BRT); }
inline bool is_full() { return digitalRead(PIN_FLA); }
