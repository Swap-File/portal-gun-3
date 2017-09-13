#include "portal.h"
#include "inputoutput.h"
#include <wiringPi.h>

#define BUCKET_SIZE 2

void inputoutput_setup(void){

	
	pinMode (PIN_FAN_PWM, PWM_OUTPUT );
	pinMode (PIN_IR_PWM, PWM_OUTPUT );
	
	pinMode (PIN_PRIMARY, INPUT);
	pinMode (PIN_ALT, INPUT);
	pinMode (PIN_MODE, INPUT);
	pinMode (PIN_RESET, INPUT);
	
	pullUpDnControl  (PIN_PRIMARY, PUD_UP);
	pullUpDnControl  (PIN_ALT, PUD_UP);
	pullUpDnControl  (PIN_MODE, PUD_UP);
	pullUpDnControl  (PIN_RESET, PUD_UP);
	
}

int inputoutput_update(int fanspeed, int ir_brightness){

	static int primary_bucket = 0;
	static int alt_bucket = 0;
	static int mode_bucket = 0;
	static int reset_bucket = 0;

	//analogWrite(PIN_FAN_PWM,fanspeed);
	//analogWrite(PIN_IR_PWM,ir_brightness);
	
	//basic bucket debounce
	if(digitalRead (PIN_PRIMARY)) primary_bucket++;
	else primary_bucket = 0;
	if(digitalRead (PIN_ALT)) alt_bucket++;
	else alt_bucket = 0;
	if(digitalRead (PIN_MODE)) mode_bucket++;
	else mode_bucket =0;
	if(digitalRead (PIN_RESET)) reset_bucket++;
	else reset_bucket = 0;
	
	if (primary_bucket > BUCKET_SIZE)	return BUTTON_PRIMARY_FIRE;
	if (alt_bucket > BUCKET_SIZE)		return BUTTON_ALT_FIRE;
	if (mode_bucket > BUCKET_SIZE)		return BUTTON_MODE_TOGGLE;
	if (reset_bucket > BUCKET_SIZE)		return BUTTON_RESET;
	return BUTTON_NONE;
}