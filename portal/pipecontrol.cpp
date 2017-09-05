#include "portal.h"
#include "pipecontrol.h"
#include "arduino.h"

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

uint32_t ping_time = 0;
uint8_t web_packet_counter = 0;
int ip;

int temp_in;
int web_in;
int gstreamer_crashes = 0;
int ahrs_crashes = 0;

FILE *bash_fp;
FILE *ahrs_fp;
FILE *gst_fp;
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
	system("pkill raspivid");
	system("pkill ahrs");
}

void pipecontrol_setup(int new_ip){
	ip = new_ip;
	
	bash_fp = popen("bash", "w");
	fcntl(fileno(bash_fp), F_SETFL, fcntl(fileno(bash_fp), F_GETFL, 0) | O_NONBLOCK);

	launch_ahrs_control();
	launch_gst_control();

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
	
	if (ip == 22)		ping_fp = popen("ping 192.168.1.23", "r");
	else if (ip == 23)	ping_fp = popen("ping 192.168.1.22", "r");
	fcntl(fileno(ping_fp), F_SETFL, fcntl(fileno(ping_fp), F_GETFL, 0) | O_NONBLOCK);
	
	//empty named pipe
	char buffer[100]; 
	while(read(web_in, buffer, sizeof(buffer)-1));

}

void ahrs_command(int x, int y, int z, int number){
	errno = 0;
	int completed = 0;
	while (completed == 0){
		fprintf(ahrs_fp, "%d %d %d\n",number,x,y);
		fflush(ahrs_fp);
		if (errno == EPIPE) {
			printf("BROKEN PIPE TO AHRS_CONTROL!\n");
			fclose(ahrs_fp);
			launch_ahrs_control();
			errno = 0;
			ahrs_crashes++;
		}else{
			completed = 1;
		}
	}
}		

void launch_ahrs_control(void){
	printf("AHRS_CONTROL: SPAWNING PROCESS\n");
	ahrs_fp = popen("/home/pi/portal/ahrs-visualizer/ahrs-visualizer", "w");
	fcntl(fileno(ahrs_fp), F_SETFL, fcntl(fileno(ahrs_fp), F_GETFL, 0) | O_NONBLOCK);
	printf("AHRS_CONTROL: READY\n");
}

void gst_command(int number){
	errno = 0;
	int completed = 0;
	while (completed == 0){
		fprintf(gst_fp, "%d\n",number);
		fflush(gst_fp);
		if (errno == EPIPE) {
			printf("BROKEN PIPE %d TO GST_CONTROL!\n",gstreamer_crashes);
			fclose(gst_fp);
			launch_gst_control();
			errno = 0;
			gstreamer_crashes++;
		}else{
			completed = 1;
		}
	}
}

void launch_gst_control(void){
	printf("GST_CONTROL : SPAWNING PROCESS\n");
	if (ip == 22) gst_fp = popen("/home/pi/portal/gstvideo/gstvideo 22", "w");
	else if (ip == 23) gst_fp = popen("/home/pi/portal/gstvideo/gstvideo 23", "w");
	fcntl(fileno(gst_fp), F_SETFL, fcntl(fileno(gst_fp), F_GETFL, 0) | O_NONBLOCK);
	fprintf(bash_fp, "sudo renice -n -20 $(pidof gstvideo)\n");
	fflush(bash_fp);
	printf("GST_CONTROL : READY\n");
}


void aplay(const char *filename){
	fprintf(bash_fp, "aplay %s &\n",filename);
	fflush(bash_fp);
}

void web_output(const this_gun_struct& this_gun,const arduino_struct& arduino ){
	FILE *webout_fp;
	webout_fp = fopen("/var/www/html/tmp/temp.txt", "w");
	fprintf(webout_fp, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %.2f %.2f %.2f %.2f %d\n" ,\
	this_gun.state_solo,this_gun.state_duo,this_gun.connected,this_gun.ir_pwm,\
	this_gun.playlist_solo[0],this_gun.playlist_solo[1],this_gun.playlist_solo[2],this_gun.playlist_solo[3],\
	this_gun.playlist_solo[4],this_gun.playlist_solo[5],this_gun.playlist_solo[6],this_gun.playlist_solo[7],\
	this_gun.playlist_solo[8],this_gun.playlist_solo[9],this_gun.effect_solo,\
	this_gun.playlist_duo[0],this_gun.playlist_duo[1],this_gun.playlist_duo[2],this_gun.playlist_duo[3],\
	this_gun.playlist_duo[4],this_gun.playlist_duo[5],this_gun.playlist_duo[6],this_gun.playlist_duo[7],\
	this_gun.playlist_duo[8],this_gun.playlist_duo[9],this_gun.effect_duo,\
	arduino.battery_level_pretty,arduino.temperature_pretty,this_gun.coretemp,\
	this_gun.latency,web_packet_counter++);
	fclose(webout_fp);
	rename("/var/www/html/tmp/temp.txt","/var/www/html/tmp/portal.txt");
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
				case WEB_ORANGE_WIFI:	web_button = BUTTON_ORANGE_SHORT;  		break;
				case WEB_BLUE_WIFI:		web_button = BUTTON_BLUE_SHORT;    		break;
				case WEB_BLUE_SELF:		web_button = BUTTON_BOTH_LONG_BLUE; 	break; 
				case WEB_ORANGE_SELF: 	web_button = BUTTON_BOTH_LONG_ORANGE; 	break; 
				case WEB_CLOSE: 		web_button = BUTTON_BLUE_LONG; 			break; 
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