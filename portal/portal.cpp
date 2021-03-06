#include "portal.h"
#include "i2cread.h"
#include "ledcontrol.h"
#include "udpcontrol.h"
#include "pipecontrol.h"
#include "statemachine.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <wiringPi.h>

volatile bool video_done = false;

void INThandler(int dummy) {
	printf("\nCleaning up...\n");
	ledcontrol_wipe();
	pipecontrol_cleanup();
	exit(1);
}

void gstvideo_handler(int dummy) {
	printf("PORTAL: Video EoS caught, closing the portal...\n");
	video_done = true;
}

int main(void){
	
	if (getuid()) {
		printf("Run as root!\n");
		exit(1);
	}

	pipecontrol_cleanup();
	
	struct this_gun_struct this_gun;
	struct other_gun_struct other_gun;

	//catch broken pipes to respawn threads if they crash
	signal(SIGPIPE, SIG_IGN);
	//catch ctrl+c when exiting
	signal(SIGINT, INThandler);
	//catch signal from gstvideo when videos are done
	signal(SIGUSR2, gstvideo_handler);
	
	//stats
	uint32_t time_start = 0;
	int missed = 0;
	uint32_t time_fps = 0;
	int fps = 0;
	uint32_t time_delay = 0;
	int changes = 0;
	
	//setup libaries
	web_output(this_gun);
	wiringPiSetup () ;
	ledcontrol_setup();
	i2creader_setup();	
	udpcontrol_setup();
	pipecontrol_setup();
	
	bool freq_50hz = true; //toggles every other cycle, cuts 100hz to 50hz
	
	while(1){
		//cycle start code - delay code
		time_start += 10;
		uint32_t predicted_delay = time_start - millis(); //calc predicted delay
		if (predicted_delay > 10) predicted_delay = 0; //check for overflow
		if (predicted_delay != 0){
			delay(predicted_delay); 
			time_delay += predicted_delay;
		}else{
			time_start = millis(); //reset timer to now
			printf("MAIN Skipping Idle...\n");
			missed++;
		}
		this_gun.clock = millis();  //stop time for duration of frame
		freq_50hz = !freq_50hz; 
		this_gun.state_duo_previous = this_gun.state_duo;
		this_gun.state_solo_previous = this_gun.state_solo;
		other_gun.state_previous = other_gun.state;
		
		//program code starts here
		update_temp(&this_gun.coretemp);
		update_ping(&this_gun.latency);
			
		//read states from buttons
		int button_event = BUTTON_NONE;	
		button_event = io_update(this_gun);

		//if no event, read from the web
		if (button_event == BUTTON_NONE) button_event = read_web_pipe(this_gun);
		//read other gun's data, only if no button events are happening this cycle

		//if still no event, read button from the other gun for processing
		while (button_event == BUTTON_NONE){
			int result = udp_receive_state(&other_gun.state,&other_gun.clock);
			if (result <= 0) break;  //read until buffer empty
			else other_gun.last_seen = this_gun.clock;  //update time data was seen
			if (millis() - this_gun.clock > 5) break; //flood protect
		}

		if (video_done){
			if (this_gun.state_solo == 4) this_gun.state_solo = 3;
			else if (this_gun.state_solo == -4) this_gun.state_solo = -3;
			video_done = false;
		}
						
		//process state changes
		local_state_engine(button_event,this_gun,other_gun);
		if (button_event != BUTTON_NONE) changes++;
		
		//gstreamer state stuff, blank it if shared state and private state are 0
		int gst_state = GST_BLANK;
		//camera preload
		if(this_gun.state_duo <= -1) gst_state = GST_RPICAMSRC;
		//project shared preload
		else if(this_gun.state_duo >= 1) gst_state = this_gun.effect_duo;		
		//project private preload
		else if(this_gun.state_solo != 0) gst_state = this_gun.effect_solo;	
		
		//ahrs effects
		int ahrs_state = AHRS_CLOSED; 
		// for networked modes
		if (this_gun.state_solo == 0){
			if (this_gun.state_duo == 3)      ahrs_state = AHRS_CLOSED_ORANGE;
			else if (this_gun.state_duo == 4) ahrs_state = AHRS_OPEN_ORANGE;		
			else if (this_gun.state_duo == 5) ahrs_state = AHRS_CLOSED_ORANGE; //blink shut on effect change
		}
		// for self modes
		if (this_gun.state_duo == 0){
			if (this_gun.state_solo == 3)       ahrs_state = AHRS_CLOSED_ORANGE;
			else if (this_gun.state_solo == -3) ahrs_state = AHRS_CLOSED_BLUE;
			else if (this_gun.state_solo <= -4) ahrs_state = AHRS_OPEN_BLUE;
			else if (this_gun.state_solo >= 4)  ahrs_state = AHRS_OPEN_ORANGE;
		}

		//OUTPUT TO gstvideo (combo video and 3d data)
		gstvideo_command(ahrs_state,gst_state,0,0,0);

		//switch off updating the leds or i2c every other cycle, each takes about 1ms
		if(freq_50hz){ 
			this_gun.brightness = led_update(this_gun,other_gun);
		}
		else{
			i2creader_update(this_gun);
		}
		
		audio_effects(this_gun);
		
		//send data to other gun
		static uint32_t time_udp_send = 0;
		if (this_gun.clock - time_udp_send > 100){
			udp_send_state(this_gun.state_duo,this_gun.clock);
			time_udp_send = this_gun.clock;
			web_output(this_gun);
		}
		
		//cycle end code - fps counter and stats
		fps++;
		if (time_fps < millis()){		
			printf("MAIN FPS:%d mis:%d idle:%d%% changes:%d \n",fps,missed,time_delay/10,changes);
			fps = 0;
			time_delay = 0;
			time_fps += 1000;
			//readjust counter if we missed a cycle
			if (time_fps < millis()) time_fps = millis() + 1000;
		}	
	}
	return 0;
}