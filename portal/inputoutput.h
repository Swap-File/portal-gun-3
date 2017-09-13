#ifndef _INPUTOUTPUT_H
#define _INPUTOUTPUT_H


#define PIN_FAN_PWM  26 //pwm0
#define PIN_IR_PWM   23 //pwm1

#define PIN_PRIMARY 2
#define PIN_ALT     3
#define PIN_MODE    4 
#define PIN_RESET   5


void inputoutput_setup(void);
int inputoutput_update(int fanspeed, int ir_brightness);

#endif