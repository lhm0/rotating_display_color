#include <Arduino.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"

#define PWM0_A 16					// enable R LEDs
#define PWM0_B 17					// enable G LEDs
#define PWM1_A 18					// enable B LEDs

#define SPI0_MISO 0					// LED SPI (STP24DP05)
#define SPI0_CSO 1					// RP2040 is SPI master
#define SPI0_SCK 2
#define SPI0_MOSI 3

#define SPI1_MOSI 8					// SPI for ESP32 - RP2040 communication
#define SPI1_CSO 9					// RP2040 is SPI slave
#define SPI1_SCK 10
#define SPI1_MISO 11

#define LE 4						// control lines for STP24DP05
#define DG 5
#define DF0 6
#define DF1 7

#define DM 12
#define EF 13
#define TF 14
#define Hall 15						// Hall sensor input

#define lpt 3840;					// lines per turn (240 pixels x 16 subpixel)

#define BUF_LEN 128
#define SPI_RW_LEN 1
#define SPI1_BLOCK_SIZE 1000

void PixelIRQ();											// declare PixelIRQ handler
void SPIIRQ();												// declare SPIIRQ handler
void HallIRQ(uint gpio, uint32_t events);					// declare HallIRQ Handler

void startPattern();										// populates line[][][] with initial values

// uint16_t pa[16] = {0x0000; 0x0080; 0x1008; 0x1088; 0x2244; 0x2492; 0x294A; 0x2AAA; 0xAAAA; 0xAAD5; 0xB6DA; 0xDB6D; 0xDBDB; 0xF7D7; 0xEFF7; 0xFF7F};

uint8_t spi0_wBuff[15];										// write buffer for SPI0
uint8_t line[3840][5][3];									// 240 x 16 = 3840 [pixel row] [5 x 8 = 40] [RGB] => intensity

uint8_t spi1_in_buf[BUF_LEN];

int LPT;												// 

int line_counter0;       									// counts the displayed lines. line_counter0 is 1/2turn ahead of line_counter1
int line_counter1 = 0;        								//
long line_period = 50000; 									// time per line
long off_period = 49900;
int line_offset;        									// clock orientation (12 is up); depends on where the trigger LED is positioned
long tpt = 0;												// time oer turn	

int SPI1_index = 0;											// index of SPI1 block transmission
bool listing = false;

void setup() {
	LPT = 3840;
	line_counter0 = LPT/2;
	line_counter1 = 0;
	line_offset = LPT*20/240;

// ====================================================================================================================
//
//										setup for STP24DP05 control lines
//
// ====================================================================================================================

	gpio_init(LE);											// set LE as output
	gpio_set_dir(LE, true);
	gpio_put(LE, 0);										// set LE to 0 => hold value

	gpio_init(DG);											// set DG as output
	gpio_set_dir(DG, true);
	gpio_put(DG, 0);										// gradual delay = 0 => activate

	gpio_init(DF0);											// set DF0 as output
	gpio_set_dir(DF0, true);
	gpio_put(DF0, 1);										// data flow: RGB

	gpio_init(DF1);											// set DF1 as output
	gpio_set_dir(DF1, true);
	gpio_put(DF1, 0);										// data flow: RGB

	gpio_init(EF);											// set DF1 as input
	gpio_set_dir(EF, false);

	gpio_init(TF);											// set DF1 as input
	gpio_set_dir(TF, false);

	gpio_init(DM);											// set DM as output
	gpio_set_dir(DM, true);
	gpio_put(DM, 1);

	gpio_init(Hall);
	gpio_set_dir(Hall, false);								// Hall is an input
	gpio_pull_up(Hall);										// switch on pull up resistor


// ====================================================================================================================
//
//										setup PWM for LED brightness control
//
// ====================================================================================================================

	gpio_init(PWM0_A);										// set gpio as output
	gpio_set_dir(PWM0_A, true);								//
	gpio_init(PWM0_B);										// set gpio as output
	gpio_set_dir(PWM0_B, true);								//
	gpio_init(PWM1_A);										// set gpio as output
	gpio_set_dir(PWM1_A, true);								//

	gpio_set_function(PWM0_A, GPIO_FUNC_PWM);				// enable PWM on 3 pins
	gpio_set_function(PWM0_B, GPIO_FUNC_PWM);
	gpio_set_function(PWM1_A, GPIO_FUNC_PWM);

	pwm_clear_irq(0);										// clear pwm interrupt
	pwm_set_irq_enabled(0, true);							// enable pwm interrupt

	irq_set_exclusive_handler(PWM_IRQ_WRAP, PixelIRQ);		// attach PixelIRQ() to the PWM interrupt
	irq_set_enabled(PWM_IRQ_WRAP, true);					// enanble irq

	pwm_set_wrap (0, line_period);							// upper threshold of counter 0
	pwm_set_phase_correct (0, false);						// counter 0 will reset to 0, once upper threshold is reached
	
	pwm_set_wrap (1, line_period);							// upper threshold of counter 1
	pwm_set_phase_correct (1, false);						// counter 1 will reset to 0, once upper threshold is reached

	pwm_set_clkdiv(0, 1.f);					    		// set divisor for clock divider (system clock = 125 MHz)
	pwm_set_clkdiv(1, 1.f);	

	pwm_set_chan_level(0, 0, off_period);					// level is the mid threshold, when the output goes to zero
	pwm_set_chan_level(0, 1, off_period);					// level is the mid threshold, when the output goes to zero
	pwm_set_chan_level(1, 0, off_period);					// level is the mid threshold, when the output goes to zero

	pwm_set_enabled (0, true);								// enable PWM slice 0 (output pins 16 and 17)
	pwm_set_enabled (1, true);								// enable PWM slice 1 (output pins 18)

	startPattern();

// ====================================================================================================================
//
//										setup SPI for data transfer to LEDs
//
// ====================================================================================================================

	spi_init(spi0,25*1000*1000);										// initialize SPI0, set baudrate to 25 MHz (maximum of STP24DP05)
	spi_set_slave(spi0, false);											// initialize SPI0 as master
	spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);		// 8 data bits, data latched on first clock transition, clock active high

	gpio_set_function(SPI0_MOSI, GPIO_FUNC_SPI);						// initialize SPI0 pins
	gpio_set_function(SPI0_CSO, GPIO_FUNC_SPI);
	gpio_set_function(SPI0_SCK, GPIO_FUNC_SPI);
	gpio_set_function(SPI0_MISO, GPIO_FUNC_SPI);


// ====================================================================================================================
//
//										setup interrupt handler for Hall trigger
//
// ====================================================================================================================

	gpio_set_function(Hall, GPIO_FUNC_SIO);								// software IO control (SIO)
	gpio_set_irq_enabled_with_callback(Hall, GPIO_IRQ_EDGE_FALL, true, &HallIRQ);

	Serial.begin(115200);
}


void setup1() {

// ====================================================================================================================
//
//										setup SPI for data transfer ESP32 => RP2040
//			  							 The SPI1 interrupt is handelt bei core1 !
//
// ====================================================================================================================

	spi_init(spi1,25*1000);												// initialize SPI0, set baudrate to 25 MHz (maximum of STP24DP05)
	spi_set_slave(spi1, true);											// initialize SPI1 as slave
//	spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);		// 8 data bits, data latched on first clock transition, clock active high

	gpio_set_function(SPI1_MOSI, GPIO_FUNC_SPI);						// initialize SPI1 pins
	gpio_set_function(SPI1_CSO, GPIO_FUNC_SPI);
	gpio_set_function(SPI1_SCK, GPIO_FUNC_SPI);
	gpio_set_function(SPI1_MISO, GPIO_FUNC_SPI);

	spi1_hw -> imsc = 0x06;								    // enable the RX FIFO interrupt (RXIM) and RXtimeout (RTIM)
	irq_set_exclusive_handler(SPI1_IRQ, SPIIRQ);			// attach SPIIRQ() to the SPI1 interrupt
	irq_set_enabled(SPI1_IRQ, true);						// enanble irq

}


void loop() {
	for (int i=0; i<100; i++) {
		Serial.print("Hello world ");
		Serial.println(gpio_get(Hall));
		delay(1000);
	}
	if (listing==true) {
			Serial.println ("data received....");
			int m=0;
			for (int k=0; k<SPI1_BLOCK_SIZE; k++) {
				if (m==0) Serial.printf ("%03d: ",k);
				Serial.printf ("%02X ", line[k]);
				m++;
				if (m==16) {
					Serial.println("");
					m=0;
				}
			}
			Serial.println("");
			listing=false;
	}
}

void PixelIRQ() {
	pwm_clear_irq(0);								// clear interrupt

	tpt = tpt + line_period;

	spi0_wBuff[0]=line[line_counter1][4][2];		//R
	spi0_wBuff[1]=line[line_counter1][4][1];		//G	
	spi0_wBuff[2]=line[line_counter1][4][0];		//B

	spi0_wBuff[3]=line[line_counter1][3][2];		//R
	spi0_wBuff[4]=line[line_counter1][3][1];		//G	
	spi0_wBuff[5]=line[line_counter1][3][0];		//B

	spi0_wBuff[6]=(line[line_counter0][2][2] & 0xF) + (line[line_counter1][2][2] & 0xF0);		//R
	spi0_wBuff[7]=(line[line_counter0][2][1] & 0xF) + (line[line_counter1][2][1] & 0xF0);		//G
	spi0_wBuff[8]=(line[line_counter0][2][0] & 0xF) + (line[line_counter1][2][0] & 0xF0);		//B
	spi0_wBuff[9]=line[line_counter0][1][2];		//R
	spi0_wBuff[10]=line[line_counter0][1][1];		//G	
	spi0_wBuff[11]=line[line_counter0][1][0];		//B
	spi0_wBuff[12]=line[line_counter0][0][2];		//R
	spi0_wBuff[13]=line[line_counter0][0][1];		//G	
	spi0_wBuff[14]=line[line_counter0][0][0];		//B
	
	int n = spi_write_blocking(spi0, spi0_wBuff, 15);		// send 15 bytes

	gpio_put(LE, 1);									// set LE to 1 => update LED output
	gpio_put(LE, 0);									// set LE to 0 => hold value

	line_counter0++;
    if (line_counter0 > LPT) line_counter0 = 0;
    line_counter1++;
    if (line_counter1 > LPT) line_counter1 = 0;

}


void HallIRQ(uint gpio, uint32_t events) {

	tpt = tpt + pwm_get_counter(0);
	long new_line_period = tpt/LPT;
  	if(labs(new_line_period-line_period)>10) {
      	line_period=new_line_period;             				// when motor starts spinning, tpt changes very much from turn to turn.
  		} 
	else {
      	line_period=(15*line_period+new_line_period)/16;        // in this case correct line_period fully. Otherwise make small corrections, only.
  	}

	if (line_period>65500) line_period = 65500;
	off_period = line_period-100;        			// 
 
  	pwm_set_wrap (0, line_period);							// upper threshold of counter 0
	pwm_set_wrap (1, line_period);							// upper threshold of counter 1

	pwm_set_chan_level(0, 0, off_period);					// level is the mid threshold, when the output goes to zero
	pwm_set_chan_level(0, 1, off_period);					// level is the mid threshold, when the output goes to zero
	pwm_set_chan_level(1, 0, off_period);					// level is the mid threshold, when the output goes to zero


  	tpt = 0;
  	line_counter1 = line_offset;  							// restart line_counter
  	line_counter0 = line_offset + 1920;
  	if (line_counter0 >= 3840) line_counter0 = line_counter0-3840;

}

void SPIIRQ() {
	/*
	while (spi_is_readable(spi1)) {							// read FIFO until empty
//  		spi_read_blocking (spi1, 0, line + SPI1_index, 1);
		SPI1_index++;
		if (SPI1_index>=SPI1_BLOCK_SIZE) {
			SPI1_index=0;
			listing=true;
		}
	}
	*/
}




void startPattern() {
	int i, j;
	for (i=0; i < (3840); i++) {
		for (j=0; j<5; j++) {
			line[i][j][0] = 0xFF;							// R
			line[i][j][1] = 0;								// G
			line[i][j][2] = 0;								// B
			line[i+1280][j][0] = 0xFF;
			line[i+1280][j][1] = 0xFF;
			line[i+1280][j][2] = 0;
 			line[i+2560][j][0] = 0;
			line[i+2560][j][1] = 0;
			line[i+2560][j][2] = 0xFF;
		}
	}
}