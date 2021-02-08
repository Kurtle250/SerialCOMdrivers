/************************************************************************/
/*
	@module I2Cdriver
	Author: Kurt Elliott
	Date: 2021-02-04 
	## I2C driver for a arduino mega 2560 Rev3
	The primary role of this project was to write a I2C driver program
	that would communication with other devices using I2C. This project
	was done to further develop my coding skills and also learn more serial
	communication protocols.                                            
/************************************************************************/
/// @file I2Cdriver.c
#include "I2Cdriver.h"
#include "avr/io.h"

/* global variables */
uint8_t TX_buffer;	///< Transmission buffer
uint8_t RX_buffer;	///< Receive buffer

/**
 * \brief Initialize I2C communication
 *  configures TWI control regisiters and sets the baudrate to 100kHz
 *  and prescaler to 1   
 */ 
void i2c_Init(void){  						
	TWCR = (1<<TWEN)|	//enable TWI,TWI interrupt, TWI acknowledge bit					
		   (1<<TWINT)|						
		   (1<<TWEA);
    TWBR = 0x21;	    //set bit rate to 0x21 for 100kHz SCL 						
	TWSR = 0x00;		//set SCL pre-scaler to 1				
}

/**
 * \brief send START condition and format slave address with read/write bit
 *  send START condition than format the slave address by shifting address left by 1 
 *  for write mode and shift by 1 and OR with 1 for read mode.
 *  error handling is checking the status regisiter to confirm the correct
 *  return status was returned
 * @param SLA = [8-bit unsigned int] 7-bit slave address s
 * @param readWrite = [8-bit unsigned int]  RW mode  
 */ 
void i2c_start(uint8_t SLA, uint8_t readWrite){
	TWCR |= START;							//send START condition for I2C communication
	while (!(TWCR & (1<<TWINT)));			//wait for slave START acknowledgment
	if ((TWSR & 0xF8) != TW_START);			//error handling

	if (readWrite == TW_WRITE){				//select read or write mode
		TWDR = (SLA<<1);                    //one shift left slave address to insure LSB 0
	}else if(readWrite == TW_READ){			 
		TWDR = (SLA<<1)|0x01;               //one shift left slave address than OR with 1 for LSB 
	}else{
		//ERROR
	}

	TWCR =  (1<<TWINT)|						//send slave address with read/write mode bit
			(1<<TWEN);						
	while (!(TWCR & (1<<TWINT)));			//wait for slave START acknowledgment
	if ((TWSR & 0xF8) != TW_MT_SLA_ACK);	//error handling
}

/**
 * \brief send data byte
 * load data into transmit buffer than data register
 * and send data byte to slave and wait for acknowledgment 
 * @param data = [8-bit unsigned int] 0xXX  
 */ 
void i2c_writeByte(uint8_t data){
    TX_buffer = data;                       //load data into tranmit buffer 
	TWDR = TX_buffer;                       //load data into data register
	TWCR =  (1<<TWINT)|						//send data byte to slave
			(1<<TWEN);			
	while (!(TWCR & (1<<TWINT)));			//wait for slave data acknowledgment
	if ((TWSR & 0xF8) != TW_MT_DATA_ACK);	//error handling
}
/**
 *  \brief sent STOP condition to terminate transmission 
 */ 
void i2c_stop(void){
	TWCR = STOP;	                        //send STOP condition terminate transmission						
}
/**
 * \brief send START condition with slave address and write mode 
 *        and wait for slave acknowledgment. Than send data byte and
 *        wait for slave acknowledgment and finally terminate tranmission
 *        with STOP condition
 *  
 * @param SLA = [8-bit unsigned int] 7-bit slave address 
 * @param data = [8-bit unsigned int] 0xXX 
 */ 
void i2c_Tx(uint8_t SLA, uint8_t Data){
	i2c_start(SLA,TW_WRITE);	            //send START condition with slave address + TW_WRITE mode			
	i2c_writeByte(Data);					//load and send byte of data
	i2c_stop();								//send STOP condition and terminate transmission
}
/**
 * \brief send START condition with slave address and read mode 
 *        and wait for slave acknowledgment. Than send NUll data byte and
 *        wait for slave acknowledgment and load data register into recieved buffer
 *        finally terminate tranmission with STOP condition and return recieved buffer
 * 
 * @param SLA = [8-bit unsigned int] 7-bit slave address
 * @return slave sent data byte in RX_buffer 
 */ 
uint8_t i2c_Rx(uint8_t SLA){
	i2c_start(SLA, TW_READ);		        //send START condition with slave address + TW_READ mode			
    i2c_writeByte(0x00);					//send null data byte
	RX_buffer = TWDR;						//load data register into receive buffer
	i2c_stop();								//send STOP condition and terminate transmission
	return RX_buffer;                       //return receive buffer
}

