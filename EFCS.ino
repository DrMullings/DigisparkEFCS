#include <Bounce2.h>


/*******************************************************************************
	TODOs
*******************************************************************************/

//TODO Hall sensor on interrupt

/*******************************************************************************
	Settings
*******************************************************************************/

/*
	Misc
*/
#define _burst 2

/*
	Pins
*/
#define PIN_TRIGGER 2								// Trigger
#define PIN_COL 1									// COL
#define PIN_BURST 5									// Burst
#define PIN_FET 0									// MosfetGate
#define PIN_FA 4									// Full Auto

/*
	Flags
*/
bool _triggermerker = false;						// trigger
bool _coloben = false;								// COL was HIGH
bool _colmerker = false;							// COL
bool _colunten = false;								// COL was LOW
bool _selektburst = false;							// Burstmode
bool _cyclecomplete = false;						// Cycles
bool _selektauto = false;							// FA
bool _burstmode = false;							// Burst
int _cnt_cycles = 0;								// cycles

Bounce triggerbouncer = Bounce();
Bounce colbouncer = Bounce();
Bounce burstbouncer = Bounce();
Bounce autobouncer = Bounce();

void setup()
{
	pinMode(PIN_TRIGGER, INPUT);
	pinMode(PIN_COL, INPUT);
	pinMode(PIN_FET, OUTPUT);						//Gearbox
	pinMode(PIN_BURST, INPUT);						//Burst
	pinMode(PIN_FA, INPUT);							//Auto
	triggerbouncer.attach(PIN_TRIGGER);
	triggerbouncer.interval(2);
	colbouncer.attach(PIN_COL);
	colbouncer.interval(1);
	burstbouncer.attach(PIN_BURST);
	burstbouncer.interval(5);
	autobouncer.attach(PIN_FA);
	autobouncer.interval(4);
}

void loop()
{
	triggerbouncer.update();
	_triggermerker = triggerbouncer.read();
	colbouncer.update();
	_colmerker = colbouncer.read();
	autobouncer.update();
	_selektauto = autobouncer.read();
	burstbouncer.update();
	_selektburst = burstbouncer.read();

	/*
		Overly complex semi auto cycling by Meeseeks
		Will be touched unethically later
		Srsly WTF?
	*/

	if ((PIN_TRIGGER == HIGH) && (_cyclecomplete = false)) {
		digitalWrite(PIN_FET, HIGH);
	}                                                               
	else
	{
		digitalWrite(PIN_FET, LOW);
	}                                                                   

	if (_colmerker == LOW) { _coloben = true; }								// Hall sensor is triggered
		                                                                        
	
	if ((_coloben == true) && (_colmerker == HIGH)) { _colunten = true; }	// Hall sensor is low after triggered

	// cycle completed, COL was on top and isn't anymore
	if ((_coloben == true) && (_colunten == true)) {                                                
		_cnt_cycles++;
		_coloben = false;
		_colunten = false;
		_cyclecomplete = true;
	}

	if ((PIN_TRIGGER == LOW) && (PIN_FET == LOW) && (_cyclecomplete = true)) {
		_cyclecomplete = false;
	}

	/*
		Full dakka mode
	*/

	if (PIN_FA == HIGH) { _selektauto = true; }

	else { (_selektauto = false); }


	if ((PIN_TRIGGER == HIGH) && (_selektauto = true)) {
		digitalWrite(PIN_FET, HIGH);
	}

	else { digitalWrite(PIN_FET, LOW); }


	if (PIN_BURST == HIGH) { (_selektburst = true); }
	else { _selektburst = false; }

	if ((_selektburst = true) && (_cnt_cycles <= _burst)) {
		_burstmode = true;
	}
	else { _burstmode = false; }

	if (((_selektburst = true) && (PIN_TRIGGER == HIGH) && (_burstmode = true))) {
		digitalWrite(PIN_FET, HIGH);
	}

	else { digitalWrite(PIN_FET, LOW); }


	if ((PIN_TRIGGER == LOW) && (_cnt_cycles >= _burst)) {
		_cnt_cycles = 0;
	}

}