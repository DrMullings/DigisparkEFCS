#include "Arduino.h"
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

/*******************************************************************************
Global variables
*******************************************************************************/

bool triggerPressed = false;
int cycleLength = 0;

void setup() {
	//Pin setup
	pinMode(PIN_TRG, INPUT);
	pinMode(PIN_COL, INPUT);
	pinMode(PIN_BRT, INPUT);
	pinMode(PIN_FLA, INPUT);
	pinMode(PIN_FET, OUTPUT);

	//initialize Pins with LOW
	digitalWrite(PIN_FET, LOW);
	digitalWrite(PIN_TRG, LOW);
	digitalWrite(PIN_BRT, LOW);
	digitalWrite(PIN_FLA, LOW);
	digitalWrite(PIN_COL, LOW);

	attachInterrupt(0, isr, RISING);
}

void loop() {
	if (triggerPressed) {

		if (is_semi()) {
			cycle();
		}
		else if (is_burst()) {
			for (int i = 0; i < BURST_CNT; i++) {
				cycle();
			}
		}
		else if (is_full()) {
#ifdef IS_AN94
			cycle();
			cycle();
			while (digitalRead(PIN_TRG)) {
				cycle();
				// 60 sec for 600 rounds
				// 0,1 sec per round
				// 100 ms per round
				// 10 RPS minimum
				delay(100-cycleLength);
			}
#else
			while (digitalRead(PIN_TRG)) {
				cycle();
		}
#endif
		}
		else {
			// we should never get here
		}

		triggerPressed = false;
	}
}

inline void cycle() {
	int startCycle = millis();
	digitalWrite(PIN_FET, HIGH);
	while (!digitalRead(PIN_COL)) {
		// if we cycle longer than 5 seconds something went wrong
		if (millis() - startCycle > 5000) {
			//error handling here
		}
	}
	digitalWrite(PIN_FET, LOW);
	int endCycle = millis();
	cycleLength = endCycle - startCycle;
}

void isr() { triggerPressed = true; }
inline bool is_semi() { return digitalRead(PIN_SEM); }
inline bool is_burst() { return digitalRead(PIN_BRT); }
inline bool is_full() { return digitalRead(PIN_FLA); }
