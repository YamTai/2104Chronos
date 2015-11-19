#include <msp430.h> 
#include <stdio.h>
#include "display.h"

#define BUTTON_STAR_PIN         (BIT2)
#define BUTTON_NUM_PIN          (BIT1)
#define BUTTON_UP_PIN           (BIT4)
#define BUTTON_DOWN_PIN         (BIT0)
#define BUTTON_BACKLIGHT_PIN    (BIT3)

#define BMP_ACC_X_LSB        	(0x82)
#define BMP_ACC_X_MSB        	(0x83)
#define BMP_ACC_Y_LSB        	(0x84)
#define BMP_ACC_Y_MSB        	(0x85)
#define BMP_ACC_Z_LSB        	(0x86)
#define BMP_ACC_Z_MSB        	(0x87)

#define BMP_GRANGE           (0x0F)	   // g Range
#define BMP_BWD              (0x10)	   // Bandwidth
#define BMP_PM               (0x11)	   // Power modes
#define BMP_SCR              (0x13)	   // Special Control Register
#define BMP_RESET            (0x14)	   // Soft reset register (writing 0xB6 causes reset)
#define BMP_ISR1             (0x16)	   // Interrupt settings register 1
#define BMP_ISR2             (0x17)	   // Interrupt settings register 2
#define BMP_IMR1             (0x19)	   // Interrupt mapping register 1
#define BMP_IMR2             (0x1A)	   // Interrupt mapping register 2
#define BMP_IMR3             (0x1B)	   // Interrupt mapping register 3

#define AS_TX_BUFFER         (UCA0TXBUF)	//	USCI A0 Transmit Buffer
#define AS_RX_BUFFER         (UCA0RXBUF)	//	USCI A0 Receive Buffer
#define AS_TX_IFG            (UCTXIFG)		//	USCI A0 Transmit interrupt flag
#define AS_RX_IFG            (UCRXIFG)		//	USCI A0 Receive interrupt flag
#define AS_IRQ_REG           (UCA0IFG)		//	USCI A0 Interrupt Flags Register
#define AS_SPI_CTL0          (UCA0CTL0)		//	USCI A0 Control Register 0
#define AS_SPI_CTL1          (UCA0CTL1)		//	USCI A0 Control Register 1
#define AS_SPI_BR0           (UCA0BR0)		//	USCI A0 Baud Rate 0 (low byte)
#define AS_SPI_BR1           (UCA0BR1)		//	USCI A0 Baud Rate 1 (high byte)

// Acceleration measurement range in g
// Valid ranges are: 2, 4, 8, 16
#define accel_range       (2u)

#if (accel_range == 2)
	u8 g_range = 0x03;
#elif (accel_range == 4)
	u8 g_range = 0x05;
#elif (accel_range == 8)
	u8 g_range = 0x08;
#elif (accel_range == 16)
	u8 g_range = 0x0C;
#endif

// Bandwidth for filtered acceleration data in Hz (Sampling rate is twice the bandwidth)
// Valid bandwidths are: 8, 16, 31, 63, 125, 250, 500, 1000
#define bandwidth   (63u)

#if (bandwidth == 8)
	u8 bw = 0x08;
#elif (bandwidth == 16)
	u8 bw = 0x09;
#elif (bandwidth == 31)
	u8 bw = 0x0A;
#elif (bandwidth == 63)
	u8 bw = 0x0B;
#elif (bandwidth == 125)
	u8 bw = 0x0C;
#elif (bandwidth == 250)
	u8 bw = 0x0D;
#elif (bandwidth == 500)
	u8 bw = 0x0E;
#elif (bandwidth == 1000)
	u8 bw = 0x0F;
#endif


// Sleep phase duration in ms
// Valid sleep phase durations are: 1, 2, 4, 6, 10, 25, 50
#define sleep_phase   (6u)

#if (sleep_phase == 1)
	u8 sleep = 0x4C;
#elif (sleep_phase == 2)
	u8 sleep = 0x4E;
#elif (sleep_phase == 4)
	u8 sleep = 0x50;
#elif (sleep_phase == 6)
	u8 sleep = 0x52;
#elif (sleep_phase == 10)
	u8 sleep = 0x54;
#elif (sleep_phase == 25)
	u8 sleep = 0x56;
#elif (sleep_phase == 50)
	u8 sleep = 0x58;
#else
	#error "Sleep phase duration not supported"
#endif


void init_accel(void);
void accel_get(void);
void accel_stop(void);
u8 get_set_reg(u8, u8, int);



u8 xyz[3];

/*
 * main.c
 */

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    lcd_init();	//	LCD Iint

    /*initialise buttons*/
    P2DIR &= ~0x1F;	// P2.0 ~ P2.4 as OP
    P2REN |= 0x1F;	// P2.0 ~ P2.4 enable resistor
    P2OUT &= ~0x1F;	// P2.0 ~ P2.4 pull down
    P2IES &= ~0x1F;	// P2.0 ~ P2.4 interrupt edge select, Rising
    P2IFG &= ~0x1F;	// P2.0 ~ P2.4 interrupt flag clear
    P2IE |= 0x1F;	// P2.0 ~ P2.4 interrupt enable

    init_accel();

    while(1){

        _BIS_SR(LPM3_bits + GIE);	// low power mode to save more powar
        __no_operation();

	}
}

void init_accel(){

	/* config SPI */

	AS_SPI_CTL0 |= UCSYNC | UCMST | UCMSB | UCCKPH;	// SPI master | 8 data bits | MSB first | clock idle low, data output on falling edge
	AS_SPI_CTL1 |= UCSSEL1;	// clock source select, SMCLK (12MHz)
	AS_SPI_BR0   = 0x02;	//	SPI clock prescaler, low byte
	AS_SPI_BR1   = 0x00; 	//	SPI clock prescaler, high byte
							//	SPI clock = SMCLK/2 = 6MHz

	AS_SPI_CTL1 &= ~UCSWRST;	// Start SPI hardware

	/* config accel */

	PJDIR |= 0x03;	//	PJ.0(power), PJ.1(CSB, Chip select SPI mode)
	PJOUT &= ~0x03;	//	PJ.0 -> VDDIO, VDD. Power down accelerometer. PJ.1 -> CSB, set to low to prevent floating pin

	P2DIR |= 0x20;	//	P2.5(OP) -> INT1
	P1DIR |= 0xE0;	//	P1.5(SDO), P1.6(SDX), P1.7(SCK) as OP

	P2OUT &= ~0x20;	//	Set to low to prevent floating pin
	P1OUT &= ~0xE0;	//	Set to low to prevent floating pin

	/*ACTUAL setting up ... */

	P2DIR &= ~0x20;	//	P2.5 as input (INT1)
	P2IES &= ~0x20;	//	P2.5 interrupt, Rising edge

	P1DIR &= ~0x20;	//	P1.5 as input (SDO)
	P1REN |= 0x20;	//	P1.5 resistor enable
	P1OUT &= ~0x20;	//	P1.5 pull down
	P1SEL |= 0xE0;	//	P1.5, P1.6, P1.7, SPI mode
	PJOUT |= 0x01;	//	PJ.0 set to 1, power on accelerometer
	PJOUT |= 0x02;	//	PJ.1(CSB) set to 1, ONLY after powering on (BMA250)

	__delay_cycles(10000);//5ms for accelerometer to initialise (change to timer)

	/* write accelerometer register configuration */

	get_set_reg(BMP_GRANGE & ~BIT8, g_range, 1);	// g Range config
	get_set_reg(BMP_BWD & ~BIT8, bw, 1);			// Bandwidth config
	get_set_reg(BMP_PM & ~BIT8, sleep, 1);			// sleep phase config

	get_set_reg(BMP_IMR2 & ~BIT8, 0x01, 1);       // map 'new data' interrupt to INT1(P2.5) pin
	get_set_reg(BMP_ISR2 & ~BIT8, 0x10, 1);       // enable 'new data' interrupt

	accel_stop();

}

void accel_start(){

//	P2IFG &= ~0x20;	//	reset P2.5 interrupt flag
//	P2IE |= 0x20;	//	enable P2.5 interrupt
	PJOUT |= 0x01;	//	power on accelerometer (PJ.0 -> VDD, VDDIO)
	PJOUT |= 0x02;	//	PJ.1(CSB) set to 1, ONLY after powering on (BMA250)
}

void accel_stop(){

//	P2IFG &= ~0x20;	//	reset P2.5 interrupt flag
//	P2IE &= ~0x20;	//	disable P2.5 interrupt
	PJOUT &= ~0x02;	//	PJ.1(CSB) set to 1, ONLY after powering on (BMA250)
	PJOUT &= ~0x01;	//	power down accelerometer (PJ.0 -> VDD, VDDIO)
}

void accel_get(){

	accel_start();

	xyz[0] = get_set_reg(BMP_ACC_X_LSB, 0, 0);
	xyz[0] = get_set_reg(BMP_ACC_X_MSB, 0, 0);

	xyz[1] = get_set_reg(BMP_ACC_Y_LSB, 0, 0);
	xyz[1] = get_set_reg(BMP_ACC_Y_MSB, 0, 0);

	xyz[2] = get_set_reg(BMP_ACC_Z_LSB, 0, 0);
	xyz[2] = get_set_reg(BMP_ACC_Z_MSB, 0, 0);
}

u8 get_set_reg(u8 address, u8 config, int RW){

	//	if RW = 1, write, else read
	u8 data;
	u16 timeout = 1000u;

	P1REN &= ~0x20;	//	disable P1.5 pull down resistor
	PJOUT &= ~0x02;	//	CSB to low, selects accelerometer

	data = AS_RX_BUFFER;	//	clear buffer by reading (read once only register)

	AS_TX_BUFFER = address;	//	transmits address

	while (!(AS_IRQ_REG & AS_RX_IFG) && (--timeout > 0));	//	waits for receive interrupt (data to be received) and or till timeout

	data = AS_RX_BUFFER;//	reads data from receive buffer

	if (RW == 0){	//	only for writing
		AS_TX_BUFFER = 0;		//	transmit configuration data
	}else

	if (RW == 1){
		AS_TX_BUFFER = config;
	}

	while (!(AS_IRQ_REG & AS_RX_IFG) && (--timeout > 0));	//	waits for receive interrupt (data to be received) and or till timeout

	data = AS_RX_BUFFER;//	reads data from receive buffer

	PJOUT |= 0x02;	//	CSB to high, deselects accelerometer
	P1REN |= 0x20;	//	enable P1.5 pull down resistor

	return data;
}


#pragma vector = PORT2_VECTOR
__interrupt void Port_2(void){

	u8 *str;

	if (P2IFG == BUTTON_STAR_PIN){
		display_chars(LCD_SEG_L1_3_0, "STAR", SEG_ON);
		accel_get();
		str = int_to_array(xyz[0], 5, 0);
		display_chars(LCD_SEG_L2_4_0, (u8 *)str, SEG_ON);
	}else
	if (P2IFG == BUTTON_NUM_PIN){
		display_chars(LCD_SEG_L1_3_0, "MODE", SEG_ON);
	}else
	if (P2IFG == BUTTON_UP_PIN){
		display_chars(LCD_SEG_L1_3_0, "  UP", SEG_ON);
		accel_get();
		str = int_to_array(xyz[1], 5, 0);
		display_chars(LCD_SEG_L2_4_0, (u8 *)str, SEG_ON);
	}
	if (P2IFG == BUTTON_DOWN_PIN){
		display_chars(LCD_SEG_L1_3_0, "  DN", SEG_ON);
		accel_get();
		str = int_to_array(xyz[2], 5, 0);
		display_chars(LCD_SEG_L2_4_0, (u8 *)str, SEG_ON);
	}

	P2IFG &= ~0x1F;	// P2.0 ~ P2.4 interrupt flag clear

}
