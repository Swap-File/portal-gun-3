#include "portal.h"
#include "i2cread.h"
#include <wiringPi.h>
#include <ads1115.h>
#include <stdio.h>
#include <stdint.h>
#include <wiringPiI2C.h>
#include <unistd.h>
#include <byteswap.h>

static int i2c_accel[3];
static int i2c_adc[4];

static int accel_fd;  //file handle
static int adc_fd;  //file handle
static int adc_channel = 0;  //what channel we are working on
static bool adc_done = true; //is a conversion in progress?

void read_acclerometer(){
	
	wiringPiI2CWrite(accel_fd, 0x80 | 0x28);

	uint8_t temp[6];
	read(accel_fd, temp,6);
	
	i2c_accel[0] = (int16_t)(temp[0] | temp[1] << 8);
	i2c_accel[1] = (int16_t)(temp[2] | temp[3] << 8);
	i2c_accel[2] = (int16_t)(temp[4] | temp[5] << 8);
	
}

void start_analog_read(){
	adc_done = false;
	uint16_t config = CONFIG_DEFAULT;

	// Setup the configuration register
	//	Set PGA/voltage range
	config &= ~CONFIG_PGA_MASK;
	config |= CONFIG_PGA_4_096V;  //set gain

	//	Set sample speed
	config &= ~CONFIG_DR_MASK;
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
	config |= CONFIG_OS_SINGLE;
	config = __bswap_16 (config);
	wiringPiI2CWriteReg16 (adc_fd, 1, config);
}

void finish_analog_read(){
	
	int16_t result = wiringPiI2CReadReg16 (adc_fd, 0);
	result = __bswap_16 (result);

	//Sometimes with a 0v input on a single-ended channel the internal 0v reference
	//can be higher than the input, so you get a negative result...
	
	if (result < 0)  result = 0;
	i2c_adc[adc_channel] = result;
	adc_channel++;
	if (adc_channel > 3) adc_channel = 0;
	
	adc_done = true;	
}

bool analog_read_ready(){
	int16_t  result =  wiringPiI2CReadReg16(adc_fd, 1);
	result = __bswap_16 (result);
	if ((result & CONFIG_OS_MASK) != 0) return true;
	return false;
}


void i2creader_setup(void){
	
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

}

void i2creader_update(this_gun_struct& this_gun){
	//uint32_t start_time = micros();
	
	if (!adc_done){
		if(analog_read_ready()) finish_analog_read();
	}
	if (adc_done) start_analog_read();
	
	read_acclerometer();
	
	//printf("Took %d microseconds!\n",micros() - start_time);	
	
	this_gun.accel[0] = i2c_accel[0];
	this_gun.accel[1] = i2c_accel[1];
	this_gun.accel[2] = i2c_accel[2];
	
	this_gun.battery_level_pretty = i2c_adc[2];
	this_gun.temperature_pretty = i2c_adc[1];
}


