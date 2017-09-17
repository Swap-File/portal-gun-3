#ifndef _PIPECONTROL_H 
#define _PIPECONTROL_H

#define AHRS_CLOSED 0
#define AHRS_OPEN_BLUE 8
#define AHRS_OPEN_ORANGE 14
#define AHRS_CLOSED_BLUE 9
#define AHRS_CLOSED_ORANGE 15

#define WEB_PRIMARY_FIRE 100
#define WEB_ALT_FIRE 101
#define WEB_MODE_TOGGLE 102
#define WEB_RESET 103

#define PIN_FAN_PWM  26 //pwm0
#define PIN_IR_PWM   23 //pwm1

#define PIN_PRIMARY 2
#define PIN_ALT     3
#define PIN_MODE    4 
#define PIN_RESET   5

void pipecontrol_setup(void);
void pipecontrol_cleanup(void);
void aplay(const char *filename);
int io_update(const this_gun_struct& this_gun);
void web_output(const this_gun_struct& this_gun); //checked
void gstvideo_command(int portal_state, int video_state,int x, int y, int z);
void audio_effects(const this_gun_struct& this_gun);
int read_web_pipe(this_gun_struct& this_gun);
void update_ping(float * ping);
void update_temp(float * ping);
#endif