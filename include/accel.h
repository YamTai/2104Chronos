
#ifndef __ACCEL_H
#define __ACCEL_H

#define BMP_ACC_X_LSB        	(0x82)	//	from BMA250 datasheet pg.35
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

#define HIGH_G_THRESHOLD	(0x26)	// threshold for high g interrupt (bma250 datasheet, pg. 46)

#define shadowing			(BIT6)	// for BMP_SCR, shadowing (reading LSB updates MSB, set to disable)

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
#define accel_range       (8u)

#if (accel_range == 2)
	static u8 g_range = 0x03;
	static u16 mgLSB = 4;	// multiplied by 100 to convert from float to int
#elif (accel_range == 4)
	static u8 g_range = 0x05;
	static u16 mgLSB = 8;
#elif (accel_range == 8)
	static u8 g_range = 0x08;
	static u16 mgLSB = 16;
#elif (accel_range == 16)
	static u8 g_range = 0x0C;
	static u16 mgLSB = 31;
#endif

// Bandwidth for filtered acceleration data in Hz (Sampling rate is twice the bandwidth)
// Valid bandwidths are: 8, 16, 31, 63, 125, 250, 500, 1000
#define bandwidth   (63u)

#if (bandwidth == 8)
	static u8 bw = 0x08;
#elif (bandwidth == 16)
	static u8 bw = 0x09;
#elif (bandwidth == 31)
	static u8 bw = 0x0A;
#elif (bandwidth == 63)
	static u8 bw = 0x0B;
#elif (bandwidth == 125)
	static u8 bw = 0x0C;
#elif (bandwidth == 250)
	static u8 bw = 0x0D;
#elif (bandwidth == 500)
	static u8 bw = 0x0E;
#elif (bandwidth == 1000)
	static u8 bw = 0x0F;
#endif


// Sleep phase duration in ms
// Valid sleep phase durations are: 1, 2, 4, 6, 10, 25, 50
#define sleep_phase   (6u)

#if (sleep_phase == 1)
	static u8 sleep = 0x4C;
#elif (sleep_phase == 2)
	static u8 sleep = 0x4E;
#elif (sleep_phase == 4)
	static u8 sleep = 0x50;
#elif (sleep_phase == 6)
	static u8 sleep = 0x52;
#elif (sleep_phase == 10)
	static u8 sleep = 0x54;
#elif (sleep_phase == 25)
	static u8 sleep = 0x56;
#elif (sleep_phase == 50)
	static u8 sleep = 0x58;
#endif

#endif

u16 xyz[3];

//void init_accel(void);
void accel_get(void);
void accel_start(void);
void accel_stop(void);
u8 get_set_reg(u8, u8, int);
int is_neg(u16);
u16 remove_sign(u16);
