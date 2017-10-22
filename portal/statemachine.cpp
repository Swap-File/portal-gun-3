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
	if( button == BUTTON_RESET){
		this_gun.initiator = false; //reset initiator
		this_gun.state_solo = 0; //reset self state
		this_gun.state_duo = 0; //reset local state
		//this_gun.mode = MODE_DUO;
	}
	else if(button == BUTTON_MODE_TOGGLE){
		this_gun.initiator = false; //reset initiator
		this_gun.state_solo = 0; //reset self state
		this_gun.state_duo = 0; //reset local state
		if (this_gun.mode == MODE_DUO){
			this_gun.mode = MODE_SOLO;
		}else if (this_gun.mode == MODE_SOLO){
			this_gun.mode = MODE_DUO;
		}
			
		//TODO: add code from V2: (button == BUTTON_BOTH_LONG_ORANGE){  and (button == BUTTON_BOTH_LONG_BLUE){
	}
	else if (button == BUTTON_PRIMARY_FIRE){
		if (this_gun.mode == MODE_DUO){
			//projector modes
			if(this_gun.state_duo == 0){
				this_gun.state_duo = 1;
			}else if(this_gun.state_duo == 1){
				this_gun.state_duo = 2;
				this_gun.initiator = true; 
			}else if(this_gun.state_duo == 2 && this_gun.initiator == true){ //if leading the call, go through all modes
				this_gun.state_duo = 3;
			}else if(this_gun.state_duo == 2 && this_gun.initiator == false){  //answer an incoming call immediately and open portal on button press
				this_gun.state_duo = 4;  
			}else if(this_gun.state_duo == 4){ //open portal
				this_gun.state_duo = 5;
			}else if(this_gun.state_duo == 5){ //go back to closed portal for effect change
				this_gun.state_duo = 4;  
			}
			//camera modes
			else if (this_gun.state_duo == -1){
				this_gun.state_duo = -2;
				this_gun.initiator = true;
			}else if (this_gun.state_duo == -2 && this_gun.initiator == true){
				this_gun.state_duo = -3;
			}else if (this_gun.state_duo == -2 && this_gun.initiator == false){
				this_gun.state_duo = -4;
			}else if (this_gun.state_duo == -3 && this_gun.initiator == false){
				this_gun.state_duo = -4;  //connection established
			}
		}else if (this_gun.mode == MODE_SOLO){
			if(this_gun.state_solo >= 0 && this_gun.state_solo < 4){
				this_gun.state_solo++;
			}else if(this_gun.state_solo == 4){
				this_gun.state_solo = 3;
			}else if(this_gun.state_solo < 0 && this_gun.state_solo > -4){
				this_gun.state_solo--;
			}else if(this_gun.state_solo == -4){
				this_gun.state_solo = -3;
			}		
		}
	}
	else if (button == BUTTON_ALT_FIRE){
		if (this_gun.mode == MODE_DUO){
			if(this_gun.state_duo == -4){ //quick swap function
				this_gun.state_duo = 4;
			}
			else if (this_gun.state_duo == 0){ //start duo camera mode
				this_gun.state_duo = -1;
			} 
		}else if (this_gun.mode == MODE_SOLO){
			if(this_gun.state_solo == 0){ //blue solo portal open
				this_gun.state_solo = -1;
			}else if(this_gun.state_solo == 3 || this_gun.state_solo == 4){//solo portal color change
				this_gun.state_solo = -3;
			}else if(this_gun.state_solo == -3 || this_gun.state_solo == -4){//solo portal color change
				this_gun.state_solo = 3;
			}			
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
			this_gun.mode = 2;
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
	if ((this_gun.state_solo == 3 && (this_gun.state_solo_previous == 4 || this_gun.state_solo_previous == -4 )) || (this_gun.state_solo == -3 && (this_gun.state_solo_previous == -4 || this_gun.state_solo_previous == 4 ))){ 
		this_gun.effect_solo = this_gun.playlist_solo[this_gun.playlist_solo_index];
		this_gun.playlist_solo_index++;
		if (this_gun.playlist_solo[this_gun.playlist_solo_index] <= -1) this_gun.playlist_solo_index = 0;
		if (this_gun.playlist_solo_index >= playlist_solo_SIZE) this_gun.playlist_solo_index = 0;
	}
}
