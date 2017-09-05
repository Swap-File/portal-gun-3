#include "i2creader.h"
#include <wiringPi.h>
#include <ads1115.h>
#include <stdio.h>
#include <stdint.h>
#include <wiringPiI2C.h>
#include <unistd.h>
#include <byteswap.h>
#include <fcntl.h>

int adc_channel = 0;
int accel_fd;
int adc_fd;
bool adc_done = true;
int accel[3];
int adc[4];

int accel_fps;
int adc_fps;
int accel_fps_saved;
int adc_fps_saved;

uint32_t fps_timer; 

void read_acclerometer(){
	
	wiringPiI2CWrite(accel_fd, 0x80 | 0x28);

	uint8_t temp[6];
	read(accel_fd, temp,6);
	
	accel[0] = (int16_t)(temp[0] | temp[1] << 8);
	accel[1] = (int16_t)(temp[2] | temp[3] << 8);
	accel[2] = (int16_t)(temp[4] | temp[5] << 8);
	
	accel_fps++;
}

void start_analog_read(){
	adc_done = false;
	uint16_t config = CONFIG_DEFAULT;

	// Setup the configuration register

	//	Set PGA/voltage range

	config &= ~CONFIG_PGA_MASK ;
	config |= CONFIG_PGA_4_096V;  //set gain

	//	Set sample speed

	config &= ~CONFIG_DR_MASK ;
	config |= CONFIG_DR_128SPS; //adc sample speed is ideally at least the accelerometer rate

	//	Set single-ended channel or differential mode

	config &= ~CONFIG_MUX_MASK ;

	switch (adc_channel){
	case 0: config |= CONFIG_MUX_SINGLE_0 ; break ;
	case 1: config |= CONFIG_MUX_SINGLE_1 ; break ;
	case 2: config |= CONFIG_MUX_SINGLE_2 ; break ;
	case 3: config |= CONFIG_MUX_SINGLE_3 ; break ;
	}

	//	Start a single conversion
	config |= CONFIG_OS_SINGLE ;
	config = __bswap_16 (config) ;
	wiringPiI2CWriteReg16 (adc_fd, 1, config) ;
}

void finish_analog_read(){
	
	int16_t result = wiringPiI2CReadReg16 (adc_fd, 0);
	result = __bswap_16 (result) ;

	//Sometimes with a 0v input on a single-ended channel the internal 0v reference
	//can be higher than the input, so you get a negative result...
	
	if (result < 0)  result = 0;
	adc[adc_channel] = result;
	adc_channel++;
	if (adc_channel > 3) adc_channel = 0;
	
	adc_fps++;
	adc_done = true;	
}

bool analog_read_ready(){
	int16_t  result ;
	result =  wiringPiI2CReadReg16 (adc_fd, 1) ;
	result = __bswap_16 (result) ;
	if ((result & CONFIG_OS_MASK) != 0) return true;
	return false;
}

int main(int argc, char *argv[]) {
	
	//accelerometer start
	accel_fd =  wiringPiI2CSetup (0x19);

	//adc start
	adc_fd =  wiringPiI2CSetup (0x48);


	//// LSM303DLHC Accelerometer

    // ODR = 0100 (50 Hz ODR)
    // LPen = 0 (normal mode)
    // Zen = Yen = Xen = 1 (all axes enabled)
	wiringPiI2CWriteReg8(accel_fd,CTRL_REG1_A, 0b01010111); //100hz
	
    // FS = 10 (8 g full scale)
    // HR = 1 (high resolution enable)
    wiringPiI2CWriteReg8(accel_fd,CTRL_REG4_A, 0b00101000);

	
	
	
	//gotta go fast.
	fcntl(adc_fd, F_SETFL, fcntl(adc_fd, F_GETFL, 0) | O_NONBLOCK);
	fcntl(accel_fd, F_SETFL, fcntl(accel_fd, F_GETFL, 0) | O_NONBLOCK);
	
	while(1) {
		
	
		if (!adc_done){
			if(analog_read_ready()) finish_analog_read();
		}
		if (adc_done) start_analog_read();
		
		read_acclerometer();
		printf("accel: %d %d %d  adc: %d %d %d %d %d %d \n\r",accel[0],accel[1],accel[2],adc[0],adc[1],adc[2],adc[3],accel_fps_saved,adc_fps_saved);
		
	
		delay(9);
		
		if (millis() - fps_timer > 1000){
			accel_fps_saved= accel_fps;
			adc_fps_saved =adc_fps;
			accel_fps=0;
			adc_fps=0;
			fps_timer = millis();
		}
	}

	return 0;
}


