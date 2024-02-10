// =========================================================================================================================================
//                                                 Rotating Color Display RD56c
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================


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

#define lpt 1920					// lines per turn

#define BUF_LEN 128
#define SPI_RW_LEN 1
#define SPI1_BLOCK_SIZE 40320		// 1920 * 7 * 3

void PixelIRQ();											// declare PixelIRQ handler
void SPIIRQ();												// declare SPIIRQ handler
void HallIRQ(uint gpio, uint32_t events);					// declare HallIRQ Handler

void startPattern();										// populates line[][][] with initial values

unsigned int brightnessCalc();								// computes off periode

uint8_t line[1921][7][3];									// [pixel row] [7x8=56 LEDs] [RGB] [alternate]
uint8_t line_rcv[1921][7][3][2];							// received data is stored in this array. [2] dimension is used for alternating use
int rcv_bank = 0;											// bank of line_rcv being used for storage of received data
int use_bank = 1;											// bank of line_rcv being used for compying to line[][][]

int brightness;												// 0...100

uint8_t spi0_wBuff[21];										// write buffer for SPI0
uint8_t spi1_in_buf[BUF_LEN];

int line_counter0 = 960;       								// counts the displayed lines. line_counter0 is 1/2turn ahead of line_counter1
int line_counter1 = 0;        								//

long LPT = lpt;												// lines per turn

unsigned int line_period = 30000; 							// time per line
unsigned int off_period = 29950;
int line_offset = 700;        								// clock orientation (12 is up); depends on where the trigger LED is positioned

long tpt = 0;												// time oer turn	

bool msg = false;									//

int SPI1_index_i = 0;										// index of SPI1 block transmission
int SPI1_index_j = 0;										//
int SPI1_index_k = 0;										//

void setup() {
	LPT = lpt;
	line_counter0=LPT/2;
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
	gpio_put(DG, 1);										// gradual delay = 0 => activate

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
//										setup PWM for LED control
//
//	at maximum speed, display spins @ 50 Hz
//  time per turn = 20ms
//	time per line = 20ms/1980 lines/turn = 10 µs
//  pwm counts per line = 10 µs * 125 MHz = 1260
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

	pwm_set_wrap (0, line_period);							// upper threshold of counter 0
	pwm_set_phase_correct (0, false);						// counter 0 will reset to 0, once upper threshold is reached
	
	pwm_set_wrap (1, line_period);							// upper threshold of counter 1
	pwm_set_phase_correct (1, false);						// counter 1 will reset to 0, once upper threshold is reached

	pwm_set_clkdiv_int_frac(0, 1, 0);					    // set divisor for clock divider (system clock = 125 MHz)
	pwm_set_clkdiv_int_frac(1, 1, 0);	

	pwm_set_chan_level(0, 0, off_period);					// level is the mid threshold, when the output goes to zero
	pwm_set_chan_level(0, 1, off_period);					// level is the mid threshold, when the output goes to zero
	pwm_set_chan_level(1, 0, off_period);					// level is the mid threshold, when the output goes to zero

	pwm_set_enabled (0, true);								// enable PWM slice 0 (output pins 16 and 17)
	pwm_set_enabled (1, true);								// enable PWM slice 1 (output pins 18)

	irq_set_exclusive_handler(PWM_IRQ_WRAP, PixelIRQ);		// attach PixelIRQ() to the PWM interrupt
	irq_set_enabled(PWM_IRQ_WRAP, true);					// enanble irq

	startPattern();

// ====================================================================================================================
//
//										setup SPI for data transfer to LEDs
//
//	bits per line: 40 LEDs x 3 = 120 bits (15 bytes)
//	SPI clock frequency: 25 MHz (maximum defined by LED driver chip)
//	time per data transmission: 120 / 25 MHz = 4.8 µs
// 	==> time for data transfer matches minimum time per line (10 µs)
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

	pwm_clear_irq(0);										// clear pwm interrupt
	pwm_set_irq_enabled(0, true);							// enable pwm interrupt

	Serial.begin(115200);
}


void setup1() {

// ====================================================================================================================
//
//										setup SPI for data transfer ESP32 => RP2040
//			  							 The SPI1 interrupt is handled bei core1 !
//
// ====================================================================================================================

	spi_init(spi1,1000*1000);												//
	spi_set_slave(spi1, true);											// initialize SPI1 as slave

	gpio_set_function(SPI1_MOSI, GPIO_FUNC_SPI);						// initialize SPI1 pins
	gpio_set_function(SPI1_CSO, GPIO_FUNC_SPI);
	gpio_set_function(SPI1_SCK, GPIO_FUNC_SPI);
	gpio_set_function(SPI1_MISO, GPIO_FUNC_SPI);

	spi1_hw -> imsc = 0x06;								    // enable the RX FIFO interrupt (RXIM) and RXtimeout (RTIM)
	irq_set_exclusive_handler(SPI1_IRQ, SPIIRQ);			// attach SPIIRQ() to the SPI1 interrupt
	irq_set_enabled(SPI1_IRQ, true);						// enanble irq

}


void loop() {
		Serial.print("brightness: ");
		Serial.println(brightness);
		delay(1000);
}

// ================================================================================================================
//
//													Pixel IRQ
//	this interrupt routine is triggered by the PWM counter, when the TOP value is reached. it outputs the new LED 
// 	data via SPI.
// 	the LED data is constantly (and asynchroyously) updated via the second SPI interface (SPIIRQ()). In order to avoid
// 	disturbances of the image, the first LED row uses the up-to-date data from line_rcv[][][][], while the second LED row
// 	uses buffered data from line[][][]. When ever new data is send to the first row, the corresponding data for the
// 	second row (which will only be needed half a turn later) is stored in the line[][][] buffer. 
// 	
// ================================================================================================================

void PixelIRQ() {

	tpt+=line_period;

	for (int b = 0; b<7; b++) {
		for (int c = 0; c<3; c++) line[line_counter0][b][c] = line_rcv[line_counter0][b][c][use_bank];
	}

	spi0_wBuff[0]=line_rcv[line_counter1][6][2][use_bank];		//R
	spi0_wBuff[1]=line_rcv[line_counter1][6][1][use_bank];		//G	
	spi0_wBuff[2]=line_rcv[line_counter1][6][0][use_bank];		//B
	spi0_wBuff[3]=line_rcv[line_counter1][5][2][use_bank];		//R
	spi0_wBuff[4]=line_rcv[line_counter1][5][1][use_bank];		//G	
	spi0_wBuff[5]=line_rcv[line_counter1][5][0][use_bank];		//B
	spi0_wBuff[6]=line_rcv[line_counter1][4][2][use_bank];		//R
	spi0_wBuff[7]=line_rcv[line_counter1][4][1][use_bank];		//G	
	spi0_wBuff[8]=line_rcv[line_counter1][4][0][use_bank];		//B
	spi0_wBuff[9]=(line[line_counter0][3][2] & 0xF) + (line_rcv[line_counter1][3][2][use_bank] & 0xF0);		//R
	spi0_wBuff[10]=(line[line_counter0][3][1] & 0xF) + (line_rcv[line_counter1][3][1][use_bank] & 0xF0);	//G
	spi0_wBuff[11]=(line[line_counter0][3][0] & 0xF) + (line_rcv[line_counter1][3][0][use_bank] & 0xF0);	//B
	spi0_wBuff[12]=line[line_counter0][2][2];		//R
	spi0_wBuff[13]=line[line_counter0][2][1];		//G	
	spi0_wBuff[14]=line[line_counter0][2][0];		//B
	spi0_wBuff[15]=line[line_counter0][1][2];		//R
	spi0_wBuff[16]=line[line_counter0][1][1];		//G	
	spi0_wBuff[17]=line[line_counter0][1][0];		//B
	spi0_wBuff[18]=line[line_counter0][0][2];		//R
	spi0_wBuff[19]=line[line_counter0][0][1];		//G	
	spi0_wBuff[20]=line[line_counter0][0][0];		//B
	
	int n = spi_write_blocking(spi0, spi0_wBuff, 21);		// send 21 bytes

	gpio_put(LE, 1);									// set LE to 1 => update LED output
	gpio_put(LE, 0);									// set LE to 0 => hold value

	line_counter0++;
    if (line_counter0 > (LPT-1)) {
		line_counter0 = 0;	
	}
	
    line_counter1++;
    if (line_counter1 > (LPT-1)) {
		line_counter1 = 0;
	}

	pwm_clear_irq(0);										// clear interrupt

}

// ====================================================================================================================
//
//														HallIRQ
//
//	The Hall sensor triggers this routine, when a full turn of the display is completed. The time-per-turn (tpt) is 
//	used to re-calculate the correct time per line (line_period). The PWM couners are updated accordingly.
//
// ====================================================================================================================


void HallIRQ(uint gpio, uint32_t events) {
	tpt = tpt + pwm_get_counter(0);

	long new_line_period = tpt/LPT;

	if (new_line_period>65500) new_line_period = 65500;

  	if(labs(new_line_period-line_period)>4) {
      	line_period = new_line_period;             				// when motor starts spinning, tpt changes very much from turn to turn.
  	} 
	else {
      	line_period = (15*line_period+new_line_period)/16;        // in this case correct line_period fully. Otherwise make small corrections, only.
  	}

//	off_period = brightnessCalc();        			// 
	off_period = (unsigned int)((float)line_period * (1.0 - ((float)brightness*(float)brightness/50000.0)));
//	if ((line_period - off_period)<(95*line_period/100)) off_period = 95*line_period/100;

  	pwm_set_wrap (0, line_period);							// upper threshold of counter 0
	pwm_set_wrap (1, line_period);							// upper threshold of counter 1

	pwm_set_chan_level(0, 0, off_period);					// level is the mid threshold, when the output goes to zero
	pwm_set_chan_level(0, 1, off_period);					// level is the mid threshold, when the output goes to zero
	pwm_set_chan_level(1, 0, off_period);					// level is the mid threshold, when the output goes to zero

 	tpt = 0;
  	line_counter1 = line_offset;  							// restart line_counter
  	line_counter0 = line_offset + LPT/2;

  	if (line_counter0 >= LPT) line_counter0 = line_counter0-LPT;
  
}


// ====================================================================================================================
//
//																SPIIRQ()
// 	interrupt routine for handling incoming data at SPI1. This routine runs in core 1 in order not to interfere with 
//	time critical PixelIRQ.
//	The received data is stored in line_rcv[][][][bank], while there are two banks (0 and 1), which are used 
//	alternatingly. This avoids the display of incomplete data.
//
// ====================================================================================================================

void SPIIRQ() {
	while (spi_is_readable(spi1)) {							// read FIFO until empty
		spi_read_blocking (spi1, 0, &line_rcv[SPI1_index_i][SPI1_index_j][SPI1_index_k][rcv_bank], 1);
		SPI1_index_i++;
		if (SPI1_index_i >= 1921) {
			SPI1_index_i=0;
			SPI1_index_j++;
			if (SPI1_index_j >= 7) {
				SPI1_index_j=0;
				SPI1_index_k++;
				if (SPI1_index_k >= 3) {
					SPI1_index_k = 0;
				}
			}
		}
		if ((SPI1_index_i == 0) & (SPI1_index_j == 0) & (SPI1_index_k == 0)) {
			brightness = (int)line_rcv[1920][0][0][rcv_bank];
			use_bank = rcv_bank;
			rcv_bank++;
			if (rcv_bank > 1) rcv_bank = 0;
		}
	}
}



void startPattern() {
	int i, j;
	for (i=0; i<(LPT/3); i++) {
		for (j=0; j<7; j++) {
			line_rcv[i][j][0][1] = 0xFF;							// R
			line_rcv[i][j][1][1] = 0;								// G
			line_rcv[i][j][2][1] = 0;								// B
			line_rcv[i+(LPT/3)][j][0][1] = 0;
			line_rcv[i+(LPT/3)][j][1][1] = 0xFF;
			line_rcv[i+(LPT/3)][j][2][1] = 0;
 			line_rcv[i+(LPT*2/3)][j][0][1] = 0;
			line_rcv[i+(LPT*2/3)][j][1][1] = 0;
			line_rcv[i+(LPT*2/3)][j][2][1] = 0xFF;
		}
	}
}


unsigned int brightnessCalc() {
	float factor = (1-(((float)brightness*(float)brightness)/10000.0+0.09)*0.1);
	Serial.print("brightness factor: ");
	Serial.println(factor);
	return (unsigned int)(line_period*factor);  
}