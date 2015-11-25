#include <cc430x613x.h>
#include "bm.h"
#include "accel.h"

void accel_start(){

	/* config SPI */

	AS_SPI_CTL0 |= UCSYNC | UCMST | UCMSB | UCCKPH;	// SPI master | 8 data bits | MSB first | clock idle low, data output on falling edge
	AS_SPI_CTL1 |= UCSSEL1;	// clock source select, SMCLK (12MHz)
	AS_SPI_BR0   = 0x02;	//	SPI clock prescaler, low byte
	AS_SPI_BR1   = 0x00; 	//	SPI clock prescaler, high byte
							//	SPI clock = SMCLK/2 = 6MHz

	AS_SPI_CTL1 &= ~UCSWRST;	// Start SPI hardware

	/* config accel */

	PJDIR |= 0x03;	//	PJ.0(power), PJ.1(CSB, Chip Select Bit? (Set to enable))

	P2DIR &= ~0x20;	//	P2.5 as input (INT1)
	P2IES &= ~0x20;	//	P2.5 interrupt, Rising edge

	P1DIR &= ~0x20;	//	P1.5 as input (from SDO, SPI Data Output)
	P1REN |= 0x20;	//	P1.5 resistor enable
	P1OUT &= ~0x20;	//	P1.5 pull down
	P1SEL |= 0xE0;	//	P1.5, P1.6, P1.7, SPI mode

	PJOUT |= 0x01;	//	PJ.0 set to high, power on accelerometer
	PJOUT |= 0x02;	//	PJ.1(CSB) set to 1 (selects accelerometer), ONLY after powering on (BMA250)

	__delay_cycles(10000);	//5ms for accelerometer to initialise (change to timer)

	/* write accelerometer register configuration */

	get_set_reg(BMP_GRANGE & ~BIT8, g_range, 1);	// g Range config
	get_set_reg(BMP_BWD & ~BIT8, bw, 1);			// Bandwidth config
	get_set_reg(BMP_PM & ~BIT8, sleep, 1);			// sleep phase config (power mode)
	get_set_reg(BMP_SCR & ~BIT8, shadowing, 1);		// disable shadowing (update MSB register and LSB register together)


//	get_set_reg(BMP_IMR2 & ~BIT8, 0x01, 1);       	// map 'new data' interrupt to INT1(P2.5) pin
//	get_set_reg(BMP_ISR2 & ~BIT8, 0x10, 1);       	// enable 'new data' interrupt
	get_set_reg(HIGH_G_THRESHOLD & ~BIT8, 0xF0, 1);	//	high-g mode threshold, 0xC0 default
	get_set_reg(BMP_IMR1 & ~BIT8, 0x02, 1);       	// map 'high-g' interrupt to INT1(P2.5) pin (bma250 datasheet, pg. 43)
	get_set_reg(BMP_ISR2 & ~BIT8, 0x01 ,1);			// enable x high-g interrupt (bma250 datasheet, pg. 43)

	P2IFG &= ~0x20;	//	reset P2.5 interrupt flag
	P2IE |= 0x20;	//	enable P2.5 interrupt

	return;
}

void accel_stop(){

	P2IFG &= ~0x20;	//	reset P2.5 interrupt flag
	P2IE &= ~0x20;	//	disable P2.5 interrupt

	PJOUT &= ~0x02;	//	PJ.1(CSB) set to 1, ONLY after powering on (BMA250)
	PJOUT &= ~0x01;	//	power down accelerometer (PJ.0 -> VDD, VDDIO)

	return;
}

void accel_get(){

	u8 temp;

	/* read bma250 datasheet pg. 16 */

	xyz[0] = get_set_reg(BMP_ACC_X_MSB, 0, 0);	//	needs to read LSB to update MSB register
	xyz[0] = xyz[0]<< 2;
	temp = (get_set_reg(BMP_ACC_Y_LSB, 0, 0) & 0xC0) >> 6;
	xyz[0] |= temp;

	xyz[1] = get_set_reg(BMP_ACC_Y_MSB, 0, 0);
	xyz[1] = xyz[1]<< 2;
	temp = (get_set_reg(BMP_ACC_Y_LSB, 0, 0) & 0xC0) >> 6;
	xyz[1] |= temp;

	xyz[2] = get_set_reg(BMP_ACC_Z_MSB, 0, 0);
	xyz[2] = xyz[2]<< 2;
	temp = (get_set_reg(BMP_ACC_Y_LSB, 0, 0) & 0xC0) >> 6;
	xyz[2] |= temp;

	for(temp = 0; temp < 3; temp++){	//	if negative
		if ((xyz[temp] & 0x0200) == 0x0200){
			xyz[temp] |= 0xFC00;
			xyz[temp] = (~xyz[temp]) | 0x8000;
			xyz[temp]++;
		}
	}

	return;
}

int is_neg(u16 data){
	return ((data & 0x8000) == 0x8000);
}

u16 remove_sign(u16 data){
	return (data & 0x7FFF);
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
