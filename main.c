#include <msp430.h>

#include <stdio.h>
#include "bm.h"
#include "display.h"
#include "accel.h"

extern u8 xyz[3];

/*
 * main.c
 */
u8 test;
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

	TA0CTL |= TASSEL_1 | MC_2 | TACLR;	//	SMCLK (TASSEL_2) | Continuous mode | timer A clear
//	TA0CCTL0 |= CM_1 | CCIS_1 | SCS | CAP | CCIE;	//	rising edge | Synchronous | Capture mode | Interrupt enable
	TA0CCR0 = 32768;
	TA0CCTL0 |= CCIE;

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
		TA0CCTL0 &= ~CCIE;
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
//		TA0CCTL0 |= CCIE;	//	set interrupt flag
		TA0CCTL0 |= CCIFG;
	}else
	if (P2IFG == BUTTON_UP_PIN){
		display_chars(LCD_SEG_L1_3_0, "  UP", SEG_ON);
		TA0CCTL0 &= ~CCIE;
		accel_get();
		str = int_to_array(xyz[1], 5, 2);
		display_chars(LCD_SEG_L2_4_0, (u8 *)str, SEG_ON);
	}
	if (P2IFG == BUTTON_DOWN_PIN){
		display_chars(LCD_SEG_L1_3_0, "  DN", SEG_ON);
		TA0CCTL0 &= ~CCIE;
		accel_get();
		str = int_to_array(xyz[2], 5, 2);
		display_chars(LCD_SEG_L2_4_0, (u8 *)str, SEG_ON);
	}

	P2IFG &= ~0x1F;	// P2.0 ~ P2.4 interrupt flag clear

}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void){

	u8 *str;
    test++;
	str = int_to_array(test, 5, 0);
	display_chars(LCD_SEG_L2_4_0, (u8 *)str, SEG_ON);
	TA0CCTL0 &= ~CCIFG;
}
