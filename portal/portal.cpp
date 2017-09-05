#include "portal.h"
#include "ledcontrol.h"
#include "udpcontrol.h"
#include "pipecontrol.h"
#include "statemachine.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <wiringPi.h>

void INThandler(int dummy) {
	printf("\nCleaning up...\n");
	ledcontrol_wipe();
	pipecontrol_cleanup();
	exit(1);
}

int main(void){
	//magic numbers of the video lengths in milliseconds
	const uint32_t video_length[12] = {24467,16767,12067,82000,30434,22900,19067,70000,94000,53167,184000,140000};
	uint32_t video_end_time = 0;
	
	struct this_gun_struct this_gun;
	struct other_gun_struct other_gun;
	//struct arduino_struct arduino;
		
	//stats
	uint32_t time_start = 0;
	int missed = 0;
	uint32_t time_fps = 0;
	int fps = 0;
	uint32_t time_udp_send = 0;
	uint32_t time_delay = 0;
	int changes = 0;
	
	//setup libaries

	ledcontrol_setup();

	
	bool freq_division = true; //toggles every other cycle
	
	while(1){
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
		freq_division = !freq_division;
this_gun.state_solo = 4;

		if(freq_division) this_gun.brightness = led_update(this_gun,other_gun);
		
		//fps counter code
		fps++;
		if (time_fps < millis()){		
			printf("MAIN FPS:%d mis:%d idle:%d%% changes:%d\n",fps,missed,time_delay/10,changes);
			fps = 0;
			time_delay = 0;
			time_fps += 1000;
			//readjust counter if we missed a cycle
			if (time_fps < millis()) time_fps = millis() + 1000;
		}	

	}
	return 0;
}