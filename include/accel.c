#include <cc430x613x.h>
#include "bm.h"
#include "accel.h"

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

	__delay_cycles(10000);//5ms for accelerometer to initialise (change to timer)
}

void accel_stop(){

//	P2IFG &= ~0x20;	//	reset P2.5 interrupt flag
//	P2IE &= ~0x20;	//	disable P2.5 interrupt

	PJOUT &= ~0x02;	//	PJ.1(CSB) set to 1, ONLY after powering on (BMA250)
	PJOUT &= ~0x01;	//	power down accelerometer (PJ.0 -> VDD, VDDIO)
}

void accel_get(){

	xyz[0] = get_set_reg(BMP_ACC_X_LSB, 0, 0);
	xyz[0] = get_set_reg(BMP_ACC_X_MSB, 0, 0);

	xyz[1] = get_set_reg(BMP_ACC_Y_LSB, 0, 0);
	xyz[1] = get_set_reg(BMP_ACC_Y_MSB, 0, 0);

	xyz[2] = get_set_reg(BMP_ACC_Z_LSB, 0, 0);
	xyz[2] = get_set_reg(BMP_ACC_Z_MSB, 0, 0);
}

int is_neg(u8 data){
	return (data & 0x80);
}

u8 get_set_reg(u8 address, u8 config, int RW){

	accel_start();

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
