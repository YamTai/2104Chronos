#include <msp430.h>

#include <stdio.h>
#include "bm.h"
#include "display.h"
#include "accel.h"

#define BUTTON_STAR_PIN         (BIT2)
#define BUTTON_NUM_PIN          (BIT1)
#define BUTTON_UP_PIN           (BIT4)
#define BUTTON_DOWN_PIN         (BIT0)
#define BUTTON_BACKLIGHT_PIN    (BIT3)

#define Accel_int1				(BIT5)

extern u16 xyz[3];

/*
 * main.c
 */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    lcd_init();	//	LCD Iint

    /*initialise buttons*/
    P2DIR &= ~0x1F;	//	P2.0 ~ P2.4 as OP
    P2REN |= 0x1F;	//	P2.0 ~ P2.4 enable resistor
    P2OUT &= ~0x1F;	//	P2.0 ~ P2.4 pull down
    P2IES &= ~0x1F;	//	P2.0 ~ P2.4 interrupt edge select, Rising
    P2IFG &= ~0x1F;	//	P2.0 ~ P2.4 interrupt flag clear
    P2IE |= 0x1F;	//	P2.0 ~ P2.4 interrupt enable

//	P2IFG &= ~0x20;	//	reset P2.5 interrupt flag
//	P2IE |= 0x20;	//	enable P2.5 interrupt

    P2SEL |= 0x08;	//	P2.3 alternate function, TA1 CCR2 CCI2A

    TA0CTL |= TASSEL_1 | ID_3 | TACLR;	// ACLK (32768Hz) | /8 | timerA clear, 4096Hz (2^16/4096), MAX 16 seconds
    TA0CCTL0 = CCIE;	//	timerA0 compare/control register 1 interrupt enable

	TA1CTL |= TASSEL_2 | MC_2 | TACLR;	//	SMCLK (26MHz) | Continuous mode | timer A clear
	TA1CCTL2 |= CM_1 | CCIS_0 | SCS | CAP | CCIE;	//	rising edge | input select CCI2A | Synchronous | Capture mode | Interrupt enable

    init_accel();



    while(1){

        _BIS_SR(LPM3_bits + GIE);	// low power mode to save more powar
        __no_operation();

	}
}

#pragma vector = PORT2_VECTOR
__interrupt void Port_2(void){

	u8 *str;

	if (P2IFG == BUTTON_STAR_PIN){
		display_chars(LCD_SEG_L1_3_0, "STAR", SEG_ON);
		accel_get();
		str = int_to_array(xyz[0], 5, 2);
		if (is_neg(xyz[0])){
			display_symbol(LCD_SYMB_ARROW_UP, SEG_OFF);
			display_symbol(LCD_SYMB_ARROW_DOWN, SEG_ON);

		}else{
			display_symbol(LCD_SYMB_ARROW_DOWN, SEG_OFF);
			display_symbol(LCD_SYMB_ARROW_UP, SEG_ON);
		}
		display_chars(LCD_SEG_L2_4_0, (u8 *)str, SEG_ON);
	}else
	if (P2IFG == BUTTON_NUM_PIN){
		display_chars(LCD_SEG_L1_3_0, "MODE", SEG_ON);
		accel_start();
	}else
	if (P2IFG == BUTTON_UP_PIN){
		display_chars(LCD_SEG_L1_3_0, "  UP", SEG_ON);
	}else
	if (P2IFG == BUTTON_DOWN_PIN){
		display_chars(LCD_SEG_L1_3_0, "  DN", SEG_ON);
		accel_stop();
	}else
	if (P2IFG == BUTTON_BACKLIGHT_PIN){

	}else
	if (P2IFG == Accel_int1){
		accel_get();
		str = int_to_array(xyz[1], 5, 2);
		display_chars(LCD_SEG_L1_3_0, "HI-G", SEG_ON);
		display_chars(LCD_SEG_L2_4_0, (u8 *)str, SEG_ON);
	}

	P2IFG &= ~0x1F;	// P2.0 ~ P2.4 interrupt flag clear
	P2IFG &= ~0x20;	// accel int1 clear

}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER_A0_CCR0_ISR(void){

	display_chars(LCD_SEG_L2_4_0, (u8 *)"000000", SEG_ON);
	TA0CCR0 = 0;
	TA0CTL |= MC_0 | TACLR;	//	stop TimerA0 in up mode
	TA0CCTL0 &= ~CCIFG;

}

#pragma vector=TIMER1_A1_VECTOR
__interrupt void TIMER_A1_CCR2_ISR(void){

	u8 *str;
	u16 rand;

	if ((TA1CCTL2 & CCIFG) == 1){	//	P2.3 interrupt (CCI2A)



		rand = TA1CCR2;
		rand ^= rand << 8;

		rand %= 0xA000;	// max of 10 seconds (4096 cycles *10)

		if (rand < 0x3000){	// min of 3 seconds (4096 *3)
			rand |= 0x3000;
		}

		str = int_to_array(rand, 5, 0);
		display_chars(LCD_SEG_L2_4_0, (u8 *)str, SEG_ON);

		TA1CCTL2 &= ~CCIFG;	// clear TA1CCTL2 interrupt

		TA0CCR0 = rand;	//	set TA0CCR0 to 'random' value
		TA0CTL |= MC_1;	//	start TimerA0 in up mode

	}

}
