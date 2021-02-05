/*
 * I2C_driver.c
 *
 * Created: 2021-02-03 9:48:52 AM
 * Author : kurtg
 */ 
#include "I2Cdriver.h"
#include "avr/io.h"


/* global variables */
uint8_t TX_buffer;
uint8_t RX_buffer;

void i2c_Init(void){
	TWBR = 0x21;							// set SCL to 100khz
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);	// enable TWI, TWI interrupt, TWI acknowledge bit
	TWSR = 0x00;							// pre-scaler set to 1
}
void i2c_start(uint8_t SLA, uint8_t readWrite){
	TWCR |= START;							// send START condition to initiate transmission
	while (!(TWCR & (1<<TWINT)));			// wait for TWINT Flag set.
	if ((TWSR & 0xF8) != TW_START);			// verify Start condition received acknowledgment
	if (readWrite == TW_WRITE){				// select Write mode
		TWDR = (SLA<<1);
	}else if(readWrite == TW_READ){			// select Read mode
		TWDR = (SLA<<1)|0x01;
	}else{
		//ERROR
	}
	TWCR = (1<<TWINT)|(1<<TWEN);			// send slave address with read or write bit selected
	while (!(TWCR & (1<<TWINT)));			// wait for TWINT flag set.
	if ((TWSR & 0xF8) != TW_MT_SLA_ACK);	// verify slave address received with acknowledgment
}

void i2c_writeByte(uint8_t data){
	TX_buffer = data;
	TWDR = TX_buffer;
	TWCR = (1<<TWINT)|(1<<TWEN);			// send data to internal register
	while (!(TWCR & (1<<TWINT)));			// wait for TWINT flag set
	if ((TWSR & 0xF8) != TW_MT_DATA_ACK);	// verify data received with acknowledgment
}

void i2c_stop(){
	TWCR = STOP;							// send STOP condition to terminate transmission
}

void i2c_Tx(uint8_t SLA, uint8_t Data){
	i2c_start(SLA,TW_WRITE);				// send START condition
	i2c_writeByte(Data);					// send data byte
	i2c_stop();								// sent STOP condition
}
// I2C read condition
uint8_t i2c_Rx(uint8_t SLA){
	i2c_start(SLA, TW_READ);				// send slave address with read bit selected
	TWDR = 0x00;							// send null RX_buffer
	TWCR = (1<<TWINT)|(1<<TWEN);			
	while (!(TWCR & (1<<TWINT)));			// wait for TWINT flag set
	if ((TWSR & 0xF8) != TW_MR_DATA_ACK);	// verify data received with acknowledgment
	RX_buffer = TWDR;						// store received data from slave
	i2c_stop();	
	return RX_buffer;						// return received data RX_buffer
}