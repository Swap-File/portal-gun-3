#include "portal.h"
#include <stdio.h>

void local_state_engine(int button, this_gun_struct& this_gun, other_gun_struct& other_gun){	

	//check for expiration of other gun
	if (this_gun.clock - other_gun.last_seen > GUN_EXPIRE) {
		if (this_gun.connected != false){
			this_gun.connected = false;
			printf("MAIN Gun Expired\n");	
		}
		other_gun.state = 0;
	}
	else {
		if(this_gun.connected == false){
			this_gun.connected = true;
			printf("MAIN Gun Connected\n");
		}
	}
		
	//button event transitions
	if( button == BUTTON_ORANGE_LONG || button == BUTTON_BLUE_LONG){
		this_gun.state_duo = 0; //reset local state
		this_gun.initiator = false; //reset initiator
		this_gun.state_solo = 0; //reset self state
	}
	else if (button == BUTTON_ORANGE_SHORT){
		if(this_gun.state_duo == 0 && this_gun.state_solo == 0){
			this_gun.state_duo = 1;
		}else if(this_gun.state_duo == 1){
			this_gun.state_duo = 2;
			this_gun.initiator = true; 
		}else if(this_gun.state_duo == 2 && this_gun.initiator == true){
			this_gun.state_duo = 3;
		}else if(this_gun.state_duo == 4){
			this_gun.state_duo = 5;
		}else if(this_gun.state_duo == 5){
			this_gun.state_duo = 4;  
		}else if(this_gun.state_duo == -4){ //swap places
			this_gun.state_duo = 4;
		}else if(this_gun.state_duo == 2 && this_gun.initiator == false){  //answer an incoming call immediately and open portal on button press
			this_gun.state_duo = 4;  
		}else if(this_gun.state_solo > 0 && this_gun.state_solo < 4){
			this_gun.state_solo++;
		}else if(this_gun.state_solo == 4 || this_gun.state_solo == -3 || this_gun.state_solo == -4){
			this_gun.state_solo = 3;
		}
	}
	else if (button == BUTTON_BLUE_SHORT){
		if (this_gun.state_duo ==0 && this_gun.state_solo == 0){
			this_gun.state_duo = -1;
		}else if (this_gun.state_duo == -1){
			this_gun.state_duo = -2;
			this_gun.initiator = true;
		}else if (this_gun.state_duo == -2 && this_gun.initiator == true){
			this_gun.state_duo = -3;
		}else if (this_gun.state_duo == -2 && this_gun.initiator == false){
			this_gun.state_duo = -4;
		}else if (this_gun.state_duo == -3 && this_gun.initiator == false){
			this_gun.state_duo = -4; //connection established
		}else if(this_gun.state_solo < 0 && this_gun.state_solo > -4){
			this_gun.state_solo--;
		}else if(this_gun.state_solo == -4 || this_gun.state_solo == 3 || this_gun.state_solo == 4){
			this_gun.state_solo = -3;
		}
	}
	else if (button == BUTTON_BOTH_LONG_ORANGE){
		if ((this_gun.state_duo == 0 && this_gun.state_solo == 0) || this_gun.state_duo == 1 || this_gun.state_duo == 2 || this_gun.state_duo == 3){
			this_gun.state_solo = this_gun.state_duo + 1;
			this_gun.state_duo_previous = this_gun.state_duo = 0;  //avoid transition changes
		}else if(this_gun.state_solo == 1 ||  this_gun.state_solo == 2 || this_gun.state_solo == 3){
			this_gun.state_solo++;
		}else if(this_gun.state_solo == -3 || this_gun.state_solo == -4 || this_gun.state_solo == 4){
			this_gun.state_solo = 3;
		}			
	}
	else if (button == BUTTON_BOTH_LONG_BLUE){
		if ((this_gun.state_duo == 0 && this_gun.state_solo == 0) || this_gun.state_duo == -1 || this_gun.state_duo == -2 || this_gun.state_duo == -3){
			this_gun.state_solo = this_gun.state_duo - 1;  //do this outside of MAX macro
			this_gun.state_solo = MAX(this_gun.state_solo,-3); //blue needs to clamp to -3 for visual consistency since we dont have a blue portal in local state -3 mode
			this_gun.state_duo_previous = this_gun.state_duo = 0;  //avoid transition changes
		}else if(this_gun.state_solo == -1 || this_gun.state_solo == -2 || this_gun.state_solo == -3){
			this_gun.state_solo--;
		}else if(this_gun.state_solo == 3 || this_gun.state_solo == 4  || this_gun.state_solo == -4){
			this_gun.state_solo = -3;
		}
	}
	
	//other gun transitions
	if (this_gun.state_solo == 0){
		if ((other_gun.state_previous >= 2 || other_gun.state_previous <= -2)  && other_gun.state == 0){
			this_gun.state_duo = 0;
		}
		if (other_gun.state_previous != -2 && other_gun.state == -2 && this_gun.initiator == false){
			this_gun.state_duo = 2;
		}
		if (other_gun.state_previous != 2 && other_gun.state == 2 && this_gun.initiator == false){
			this_gun.state_duo = -2;
		}
		if (other_gun.state_previous != 3 && other_gun.state == 3 && this_gun.initiator == false){
			this_gun.state_duo = -3;
		}
		if (other_gun.state_previous != -3 && other_gun.state == -3 && this_gun.initiator == false){
			this_gun.state_duo = 2;
		}	
		if (other_gun.state_previous != -4 && other_gun.state == -4 && this_gun.initiator == true){
			this_gun.state_duo = 4;
			this_gun.initiator = false;
		}
		if (other_gun.state_previous != 4 && other_gun.state == 4 && this_gun.initiator == true){
			this_gun.state_duo = -4;
			this_gun.initiator = false;
		}
		if (other_gun.state_previous == -4 && other_gun.state >= 4 ){
			this_gun.state_duo = -4;
			this_gun.initiator = false;
		}	
	}else{
		//code to pull out of self state
		if ((other_gun.state_previous != other_gun.state)&& (other_gun.state <= -2)){
			if (this_gun.state_solo <= -3 || this_gun.state_solo >= 3){
				this_gun.state_duo = 4;
			}else{
				this_gun.state_duo = 2;
			}
			this_gun.state_solo = 0;
		}
	}
	
	//reset playlists to the start
	//continually reload playlist if in state 0 to catch updates
	if (this_gun.state_duo == 0){
		this_gun.effect_duo = this_gun.playlist_duo[0];
		if (this_gun.effect_duo <= -1) this_gun.effect_duo = GST_VIDEOTESTSRC;
		this_gun.playlist_duo_index = 1;
		//special case of playlist 1 item long
		if (this_gun.playlist_duo[this_gun.playlist_duo_index] <= -1) this_gun.playlist_duo_index = 0;
	}
	if (this_gun.state_solo == 0){
		this_gun.effect_solo = this_gun.playlist_solo[0];
		if (this_gun.effect_solo <= -1) this_gun.effect_solo = GST_VIDEOTESTSRC;
		this_gun.playlist_solo_index = 1;	
		//special case of playlist 1 item long
		if (this_gun.playlist_solo[this_gun.playlist_solo_index] <= -1) this_gun.playlist_solo_index = 0;
	}
	
	//load next shared playlist item
	if (this_gun.state_duo == 5 && this_gun.state_duo_previous == 4){ 
		this_gun.effect_duo = this_gun.playlist_duo[this_gun.playlist_duo_index];
		this_gun.playlist_duo_index++;
		if (this_gun.playlist_duo[this_gun.playlist_duo_index] <= -1) this_gun.playlist_duo_index = 0;
		if (this_gun.playlist_duo_index >= playlist_duo_SIZE) this_gun.playlist_duo_index = 0;
	}
	
	//load next private playlist item
	if ((this_gun.state_solo == 3 && (this_gun.state_solo_previous == 4 || this_gun.state_solo_previous == -4 || this_gun.state_solo_previous == -3)) || (this_gun.state_solo == -3 && (this_gun.state_solo_previous == -4 || this_gun.state_solo_previous == 4 || this_gun.state_solo_previous == 3))){ 
		this_gun.effect_solo = this_gun.playlist_solo[this_gun.playlist_solo_index];
		this_gun.playlist_solo_index++;
		if (this_gun.playlist_solo[this_gun.playlist_solo_index] <= -1) this_gun.playlist_solo_index = 0;
		if (this_gun.playlist_solo_index >= playlist_solo_SIZE) this_gun.playlist_solo_index = 0;
	}
}
