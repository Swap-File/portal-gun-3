#include "portal.h"
#include "pipecontrol.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>  
#include <wiringPi.h>

#define BUCKET_SIZE 2

uint32_t ping_time = 0;
uint8_t web_packet_counter = 0;

int temp_in;
int web_in;
int gstreamer_crashes = 0;
int ahrs_crashes = 0;

FILE *bash_fp;
FILE *gstvideo_fp;
FILE *ping_fp;

uint8_t temp_index = 0;
uint32_t temp_array[256];
uint32_t temp_total;
uint32_t temp_level;
bool temp_first_cycle = true;
					
void pipecontrol_cleanup(void){
	printf("KILLING OLD PROCESSES\n");
	system("pkill gst");
	system("pkill mjpeg");
	system("pkill fbvideo");
}


void pipecontrol_setup(){
	
	piHiPri(90);
    system("/home/pi/portal/fbvideo/fbvideo &");
    delay(2000);
	system("/home/pi/portal/gstvideo/start.sh &");
	delay(8000);
	
	gstvideo_fp = fopen("/home/pi/GSTVIDEO_IN_PIPE", "w");
	int gstvideo_fp_int = fileno(gstvideo_fp);
	fcntl(gstvideo_fp_int, F_SETFL, O_NONBLOCK);
	
	bash_fp = popen("bash", "w");
	fcntl(fileno(bash_fp), F_SETFL, fcntl(fileno(bash_fp), F_GETFL, 0) | O_NONBLOCK);

  
 
	mkfifo ("/home/pi/FIFO_PIPE", 0777 );
	
	if ((web_in = open ("/home/pi/FIFO_PIPE",  ( O_RDONLY | O_NONBLOCK))) < 0) {
		perror("WEB_IN: Could not open named pipe for reading.");
		exit(-1);
	}
	
	fprintf(bash_fp, "sudo chown www-data /home/pi/FIFO_PIPE\n");
	fflush(bash_fp);
	
	if ((temp_in = open ("/sys/class/thermal/thermal_zone0/temp",  ( O_RDONLY | O_NONBLOCK))) < 0) {
		perror("TEMP_IN: Could not open named pipe for reading.");
		exit(-1);
	}
		
	system("LD_LIBRARY_PATH=/usr/local/lib mjpg_streamer -i 'input_file.so -f /var/www/html/tmp -n snapshot.jpg' -o 'output_http.so -w /usr/local/www' &");
	
	if(getenv("GORDON")) 		ping_fp = popen("ping 192.168.1.23", "r");
	else if(getenv("CHELL")) 	ping_fp = popen("ping 192.168.1.22", "r");
	else {
		printf("SET THE GORDON OR CHELL ENVIRONMENT VARIABLE!");
		exit(1);
	}
	
	fcntl(fileno(ping_fp), F_SETFL, fcntl(fileno(ping_fp), F_GETFL, 0) | O_NONBLOCK);
	
	//empty named pipe
	char buffer[100]; 
	while(read(web_in, buffer, sizeof(buffer)-1));
	
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

void gstvideo_command(int portal_state, int video_state,int x, int y, int z){
	fprintf(gstvideo_fp, "%d %d %d %d %d\n",portal_state,video_state,x,y,z);
	fflush(gstvideo_fp);
	if (errno == EPIPE) {
		printf("BROKEN PIPE TO gstvideo_command!\n");
	}
}		

void aplay(const char *filename){
	//use dmix for alsa so sounds can play over each other
	fprintf(bash_fp, "aplay -D plug:dmix %s &\n",filename);
	fflush(bash_fp);
}

void web_output(const this_gun_struct& this_gun ){
	
	FILE *webout_fp;
	
	//send full playlist for editing
	webout_fp = fopen("/var/www/html/tmp/temp.txt", "w");
	fprintf(webout_fp, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %.2f %.2f %.2f %.2f %d %d\n" ,\
	this_gun.state_solo,this_gun.state_duo,this_gun.connected,this_gun.ir_pwm,\
	this_gun.playlist_solo[0],this_gun.playlist_solo[1],this_gun.playlist_solo[2],this_gun.playlist_solo[3],\
	this_gun.playlist_solo[4],this_gun.playlist_solo[5],this_gun.playlist_solo[6],this_gun.playlist_solo[7],\
	this_gun.playlist_solo[8],this_gun.playlist_solo[9],this_gun.effect_solo,\
	this_gun.playlist_duo[0],this_gun.playlist_duo[1],this_gun.playlist_duo[2],this_gun.playlist_duo[3],\
	this_gun.playlist_duo[4],this_gun.playlist_duo[5],this_gun.playlist_duo[6],this_gun.playlist_duo[7],\
	this_gun.playlist_duo[8],this_gun.playlist_duo[9],this_gun.effect_duo,\
	this_gun.battery_level_pretty,this_gun.temperature_pretty,this_gun.coretemp,\
	this_gun.latency,web_packet_counter,this_gun.mode);
	fclose(webout_fp);
	rename("/var/www/html/tmp/temp.txt","/var/www/html/tmp/portal.txt");
	
	//dont send full playlist, only active and pending effect
	webout_fp = fopen("/var/www/html/tmp/temp.txt", "w");
	fprintf(webout_fp, "%d %d %d %d %d %d %d %.2f %.2f %.2f %.2f %d %d\n" ,\
	this_gun.state_solo,this_gun.state_duo,this_gun.connected,\
	this_gun.playlist_solo[this_gun.playlist_solo_index],this_gun.effect_solo,\
	this_gun.playlist_duo[this_gun.playlist_duo_index],this_gun.effect_duo,\
	this_gun.battery_level_pretty,this_gun.temperature_pretty,this_gun.coretemp,\
	this_gun.latency,web_packet_counter,this_gun.mode);
	fclose(webout_fp);
	rename("/var/www/html/tmp/temp.txt","/var/www/html/tmp/fd1.txt");
	
	web_packet_counter++;
}

int read_web_pipe(this_gun_struct& this_gun){
	int web_button = BUTTON_NONE;
	int count = 1;
	char buffer[100];
	//stdin is line buffered so we can cheat a little bit
	while (count > 0){ // dump entire buffer
		count = read(web_in, buffer, sizeof(buffer)-1);
		if (count > 1){ //ignore blank lines
			buffer[count-1] = '\0';
			//keep most recent line
			int tv[11];
			printf("MAIN Web Command: '%s'\n",buffer);
			int results = sscanf(buffer,"%d %d %d %d %d %d %d %d %d %d %d", &tv[0],&tv[1],&tv[2],&tv[3],&tv[4],&tv[5],&tv[6],&tv[7],&tv[8],&tv[9],&tv[10]);
			//button stuff
			if (tv[0] == 1 && results == 2) {
				switch (tv[1]){
				case WEB_PRIMARY_FIRE:	web_button = BUTTON_PRIMARY_FIRE;  	break;
				case WEB_ALT_FIRE:		web_button = BUTTON_ALT_FIRE;    	break;
				case WEB_MODE_TOGGLE:	web_button = BUTTON_MODE_TOGGLE; 	break; 
				case WEB_RESET:     	web_button = BUTTON_RESET; 			break; 
				default: 				web_button = BUTTON_NONE;
				}
			}
			
			//pad out array with repeat (-1) once encountered
			if (results == 11){
				uint8_t filler = 0;
				for (uint8_t i = 1; i < 11; i++){
					if (tv[i] == -1) filler = -1;
					if (filler == -1) tv[i] = -1;
				}
			}
			
			//ir stuff
			if (tv[0] == 2 && results == 2) {
				if (tv[1] >= 0 && tv[1] <= 255) this_gun.ir_pwm = tv[1];
			}
			//self playlist setting
			else if (tv[0] == 3 && results == 11) {
				for (uint8_t i = 0; i < 10; i++){
					this_gun.playlist_solo[i] = tv[i+1];
				}
			}
			//shared playlist setting
			else if (tv[0] == 4 && results == 11) {
				for (uint8_t i = 0; i < 10; i++){
					this_gun.playlist_duo[i] = tv[i+1];
				}
			}
			
		}
	}
	return web_button;
}

void update_ping(float * ping){	
	int count = 1;
	char buffer[100];
	//stdin is line buffered so we can cheat a little bit
	while (count > 0){ // dump entire buffer
		count = read(fileno(ping_fp), buffer, sizeof(buffer)-1);
		if (count > 1){ //ignore blank lines
			buffer[count-1] = '\0';
			int placemarker = 0;
			int equals_pos = 0;
			while (placemarker < count-2){
				if (buffer[placemarker] == '=') equals_pos++;
				placemarker++;
				if (equals_pos == 3){
					sscanf(&buffer[placemarker],"%f", ping);
					ping_time = millis();
					break;
				}
			}
		}
	}
	if (millis() - ping_time > 2000) *ping = 0.0;
	return;
}

void update_temp(float * temp){	
	int count = 1;
	char buffer[100];
	//stdin is line buffered so we can cheat a little bit
	while (count > 0){ // dump entire buffer
		count = read(temp_in, buffer, sizeof(buffer)-1);
		if (count > 0){ //ignore blank lines
			buffer[count-1] = '\0';
			int number;
			sscanf(buffer,"%d", &number);
			if (number > 0 && number  < 85000){
				number = (number * 9/5) + 32000;
				
				if (temp_first_cycle){
					//preload filters with data if empty
					for ( int i = 0; i < 256; i++ ) temp_array[i] = number;
					temp_total = 256*number;
					temp_level = number;
					temp_first_cycle = false;
				}
				
				temp_total -= temp_array[temp_index];
				temp_array[temp_index] = number;
				temp_total += temp_array[temp_index++];
				
				//if (temp_index>63) temp_index = 0;
				
				float temp_temp = ((float)(temp_total >> 8)) / (1000.0);
				*temp = temp_temp;
				//*temp = *temp * .5 + .5 *temp_temp;
			}
		}
	}
	lseek(temp_in,0,SEEK_SET);
	return;
}

int io_update(const this_gun_struct& this_gun){
	
	pwmWrite (PIN_FAN_PWM,this_gun.fan_pwm);
	pwmWrite (PIN_IR_PWM,this_gun.ir_pwm);
		
	static int primary_bucket = 0;
	static int alt_bucket = 0;
	static int mode_bucket = 0;
	static int reset_bucket = 0;

	//basic bucket debounce
	if(digitalRead (PIN_PRIMARY) == 0) primary_bucket++;
	else primary_bucket = 0;
	if(digitalRead (PIN_ALT) == 0)     alt_bucket++;
	else alt_bucket = 0;
	if(digitalRead (PIN_MODE) == 0)    mode_bucket++;
	else mode_bucket = 0;
	if(digitalRead (PIN_RESET) == 0)   reset_bucket++;
	else reset_bucket = 0;
	
	if (primary_bucket > BUCKET_SIZE)	return BUTTON_PRIMARY_FIRE;
	if (alt_bucket > BUCKET_SIZE)		return BUTTON_ALT_FIRE;
	if (mode_bucket > BUCKET_SIZE)		return BUTTON_MODE_TOGGLE;
	if (reset_bucket > BUCKET_SIZE)		return BUTTON_RESET;
	return BUTTON_NONE;	
}
void audio_effects(const this_gun_struct& this_gun){
	
	//LOCAL STATES
	if ((this_gun.state_duo_previous != 0 || this_gun.state_solo_previous != 0) && (this_gun.state_duo == 0 && this_gun.state_solo == 0)){
		aplay("/home/pi/portalgun/portal_close1.wav");
	}
	//on entering state 1
	else if ((this_gun.state_duo_previous != this_gun.state_duo) && (this_gun.state_duo == 1)){
		aplay("/home/pi/physcannon/physcannon_charge1.wav");
	}
	//on entering state -1
	else if ((this_gun.state_duo_previous != -1)&& (this_gun.state_duo == -1)){
		aplay("/home/pi/physcannon/physcannon_charge1.wav");			
	}
	//on entering state 2
	else if ((this_gun.state_duo_previous !=2 ) &&  (this_gun.state_duo == 2)){
		aplay("/home/pi/physcannon/physcannon_charge2.wav");		
	}
	//on entering state -2 from -1
	else if ((this_gun.state_duo_previous !=-2 ) &&  (this_gun.state_duo == -2)){
		aplay("/home/pi/physcannon/physcannon_charge2.wav");
	}	
	else if ((this_gun.state_duo_previous !=-3 ) &&  (this_gun.state_duo == -3)){
		aplay("/home/pi/physcannon/physcannon_charge3.wav");
	}	
	//on entering state 3 from 2
	else if ((this_gun.state_duo_previous == 2 ) && ( this_gun.state_duo ==3)){	
		aplay("/home/pi/portalgun/portalgun_shoot_blue1.wav");
	}
	//on quick swap to rec
	else if ((this_gun.state_duo_previous < 4 )&& (this_gun.state_duo == 4)){
		aplay("/home/pi/portalgun/portal_open2.wav");
	}
	//on quick swap to transmit
	else if ((this_gun.state_duo_previous >= 4 )&& (this_gun.state_duo <= -4)){
		aplay("/home/pi/portalgun/portal_fizzle2.wav");
	}	

	//shared effect change close portal end sfx
	else if (this_gun.state_duo_previous == 4 && this_gun.state_duo == 5){
		aplay("/home/pi/portalgun/portal_close1.wav");
	}	
	//shared effect change open portal end sfx
	else if (this_gun.state_duo_previous == 5 && this_gun.state_duo == 4){
		aplay("/home/pi/portalgun/portal_open1.wav");
	}	
	
	//SELF STATES
	else if ((this_gun.state_solo_previous != this_gun.state_solo) && (this_gun.state_solo == 1 || this_gun.state_solo == -1)){
		aplay("/home/pi/physcannon/physcannon_charge1.wav");
	}
	//on entering state 2 or -2
	else if ((this_gun.state_solo_previous != this_gun.state_solo) && (this_gun.state_solo == 2 || this_gun.state_solo == -2)){
		aplay("/home/pi/physcannon/physcannon_charge2.wav");				
	}
	
	//on entering state 3 or -3 from 0
	else if ((this_gun.state_solo_previous < 3 && this_gun.state_solo_previous > -3  ) && (this_gun.state_solo == 3 || this_gun.state_solo == -3)){
		aplay("/home/pi/portalgun/portalgun_shoot_blue1.wav");
	}

	//on quick swap 
	else if (( this_gun.state_solo_previous >= 3 && this_gun.state_solo == -3) || (this_gun.state_solo_previous <= -3 && this_gun.state_solo == 3)){
		aplay("/home/pi/portalgun/portal_open2.wav");		
	}
	
	//private end sfx
	else if ((this_gun.state_solo_previous > 3 && this_gun.state_solo == 3) || (this_gun.state_solo_previous < -3 && this_gun.state_solo == -3)){
		aplay("/home/pi/portalgun/portal_close1.wav");
	}
	
	//private start sfx
	else if ((this_gun.state_solo_previous != -4 && this_gun.state_solo == -4) || (this_gun.state_solo_previous != 4 && this_gun.state_solo == 4)){
		aplay("/home/pi/portalgun/portal_open1.wav");
	}
	
	//rip from private to shared mode sfx
	else if ((this_gun.state_solo_previous <= -3 || this_gun.state_solo_previous>=3) && this_gun.state_duo == 4){
		aplay("/home/pi/portalgun/portal_open2.wav");		
	}
	
	return;
}