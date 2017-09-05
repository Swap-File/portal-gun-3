#ifndef _PIPECONTROL_H 
#define _PIPECONTROL_H

#define AHRS_CLOSED 9
#define AHRS_OPEN_BLUE 0
#define AHRS_OPEN_ORANGE 6
#define AHRS_CLOSED_BLUE 1
#define AHRS_CLOSED_ORANGE 7

void pipecontrol_setup(int ip);
void pipecontrol_cleanup(void);
void aplay(const char *filename);
//void web_output(const this_gun_struct& this_gun,const arduino_struct& arduino);
void ahrs_command(int x, int y, int z, int number);
void gst_command(int number);
void launch_gst_control(void);
void launch_ahrs_control(void);
void audio_effects(const this_gun_struct& this_gun);
int read_web_pipe(this_gun_struct& this_gun);
void update_ping(float * ping);
void update_temp(float * ping);
#endif