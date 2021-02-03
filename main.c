/*
 * I2C_driver.c
 *
 * Created: 2021-02-03 9:48:52 AM
 * Author : kurtg
 */ 

#include "avr\interrupt.h"
#include "avr/io.h"

#define START (1<<TWSTA)
#define STOP (1<<TWSTO)
#define ERROR_S() PORTL = 0x80;
#define ERROR_T() PORTL = 0x40;
#define ERROR_D() PORTL = 0x20;
#define MT_SLD_STR 0x08
#define MT_SLA_ACK  0x18
#define MT_DATA_ACK  0x01
#define LCD_I2C_ADDR 0x27	// I2C LCD  address
#define NANO_I2C_ADDR 0x08	// I2C Nano  address

uint8_t TX_buffer;
uint8_t RX_buffer;
/* global variables */
volatile unsigned int count = 0;
volatile unsigned int SLA_W = 0b00110110;

void i2c_Init(void){
	TWBR = 0x21;								// set SCL to 100khz
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);		// enable TWI, TWI interrupt, TWI acknowledge bit
	TWSR = 0x00;								// pre-scaler set to 1
}
void i2c_start(uint8_t SLA, unsigned char readWrite){
	TWCR |= START;						// Send START condition
	while (!(TWCR & (1<<TWINT)));		// Wait for TWINT Flag set.
	if ((TWSR & 0xF8) != MT_SLD_STR)ERROR_S();
	if (readWrite == 0){
		TWDR = (SLA<<1);
	}else{
		TWDR = (SLA<<1)|0x01;
	}
	TWCR = (1<<TWINT)|(1<<TWEN);		// send slave address with read or write bit selected
	while (!(TWCR & (1<<TWINT)));		// wait for TWINT flag set.
	if ((TWSR & 0xF8) != MT_SLA_ACK)ERROR_T();
}

void i2c_writeByte(uint8_t data, uint8_t intReg){
	TWDR = intReg;
	TWCR = (1<<TWINT)|(1<<TWEN);	// send internal register address
	while (!(TWCR & (1<<TWINT)));	// wait for TWINT flag set
	if ((TWSR & 0xF8) !=MT_DATA_ACK)ERROR_D();
	TWDR = data;
	TWCR = (1<<TWINT)|(1<<TWEN);	// send data to internal register
	while (!(TWCR & (1<<TWINT)));	// wait for TWINT flag set
	if ((TWSR & 0xF8) !=MT_DATA_ACK)ERROR_D();
}
// I2C read condition
uint8_t i2c_read(uint8_t addr){
	i2c_start(addr, 1);
	TWDR = (RX_buffer & 0x0);
	TWCR = (1<<TWINT)|(1<<TWEN);	// send null tx buffer
	while (!(TWCR & (1<<TWINT)));	// wait for TWINT flag set
	if ((TWSR & 0xF8) !=MT_DATA_ACK)ERROR_D();
	RX_buffer = TWDR;
	return RX_buffer;
}
	
i2c_stop(){
	TWCR = STOP;
}

// Timer 1 configured for millisecond timer delay function
void mTimer(int count){

	int i = 0;							// initial loop counter to 0
	
	TCCR1B |= (1<<WGM12)|(1<<CS11);	// Set the waveform generation mode bit description to clear timer
	// on compare math mode (CTC) and Set CS pre-scale to 1/8
	
	OCR1A = 0x03E8;						// Set output to compare register for 1000 cycles = 1ms
	
	TCNT1 = 0x0000;						// set initial value of timer counter to 0x0000;
	
	TIFR1 |= (1<<OCF1A);				// Clear timer interrupts flag and start new timer
	
	while(i < count){
		if((TIFR1 & 0x02) == 0x02){
			TIFR1 |= (1<<OCF1A);		// clear interrupt flag
			i++;						// increment loop counter
		} // end if
	} // end while
	return;
}// milliTimer

// Timer 4 configured for microsecond timer delay function
void uTimer(int count){

	int i = 0;							// initial loop counter to 0

	TCCR4B |= (1<<WGM12)|(1<<CS11);	// Set the waveform generation mode bit description to clear timer
	// on compare math mode (CTC) and Set CS pre-scale to 1/8
	OCR4A = 0x0064;						// Set output to compare register for 100 cycles = 0.1ms
	
	TCNT4 = 0x0000;						// set initial value of timer counter to 0x0000;
	
	TIFR4 |= (1<<OCF4A);				// Clear timer interrupts flag and start new timer
	
	while(i < count){
		if((TIFR4 & 0x02) == 0x02){
			TIFR4 |= (1<<OCF4A);		// clear interrupt flag
			i++;						// increment loop counter
		} // end if
	} // end while
	return;
}// microTimer

/* main routine */
int main(){	
	// System clock 
	CLKPR = (1<<CLKPCE); // Clock Pre-scaler Change Enable
	CLKPR = (1<<CLKPS0); // Set CPU clock to 8MHz from 16MHz 

	// I/O pin initializations
	DDRA = 0xF0;		// PORT A set Pins 0-3 data input & 4-7 output
	DDRB = 0xFF;		// PORT B set Pin 0-3 for stepper motor 
	DDRC = 0xFF;		// PORT C Pins 0-7 for LCD
	DDRD = 0xF0;		// PORT D pins 0-7 
	PORTD = 0x03;
	MCUCR = (~PUD);
	DDRL = 0xFF;		// PORT L Pins 0-7 output Debugging
	DDRF = 0x00;		// PORT F Pins analog input
	i2c_Init();
	i2c_start(NANO_I2C_ADDR,0);	// write to I2C nano address
	i2c_writeByte(0xFF,0x07);	// write to Nano DDRC reg and set pins high for output
	i2c_stop();
	mTimer(5);
	i2c_start(NANO_I2C_ADDR,0);	// write to I2C nano address
	i2c_writeByte(0xFF,0x08);
	i2c_stop();
	mTimer(1000);
	i2c_start(NANO_I2C_ADDR,0);	// write to I2C nano address
	i2c_writeByte(0x0,0x08);
	i2c_stop();
	while(1){// polling
	i2c_start(NANO_I2C_ADDR,0);	// write to I2C nano address
	i2c_writeByte(0xFF,0x08);
	i2c_stop();
	mTimer(1000);
	i2c_start(NANO_I2C_ADDR,0);	// write to I2C nano address
	i2c_writeByte(0x0,0x08);
	i2c_stop();	
	}//while
	return(0);
}/* main */

