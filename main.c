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

extern u8 xyz[3];

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

    P2SEL |= 0x08;

    TA0CTL |= TASSEL_1 | ID_2 | TACLR;
    TA0CCTL0 = CCIE;

	TA1CTL |= TASSEL_2 | MC_2 | TACLR;	//	SMCLK (TASSEL_2) | Continuous mode | timer A clear
	TA1CCTL2 |= CM_1 | CCIS_0 | SCS | CAP | CCIE;	//	rising edge | input select CCI0A | Synchronous | Capture mode | Interrupt enable

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
		TA0CCTL0 |= CCIS_0;
	}else
	if (P2IFG == BUTTON_UP_PIN){
		display_chars(LCD_SEG_L1_3_0, "  UP", SEG_ON);
		TA0CCTL0 &= ~CCIE;
		accel_get();
		str = int_to_array(xyz[1], 5, 2);
		display_chars(LCD_SEG_L2_4_0, (u8 *)str, SEG_ON);
	}else
	if (P2IFG == BUTTON_DOWN_PIN){
		display_chars(LCD_SEG_L1_3_0, "  DN", SEG_ON);
		TA0CCTL0 &= ~CCIE;
		accel_get();
		str = int_to_array(xyz[2], 5, 2);
		display_chars(LCD_SEG_L2_4_0, (u8 *)str, SEG_ON);
	}else
	if (P2IFG == BUTTON_BACKLIGHT_PIN){
		display_chars(LCD_SEG_L1_3_0, "  BL", SEG_ON);
	}

	P2IFG &= ~0x1F;	// P2.0 ~ P2.4 interrupt flag clear

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

    rand = TA1CCR2;
    rand ^= rand << 8;

    while (rand < 10000){
    	++rand << 1;
    }

	str = int_to_array(rand, 5, 0);
	display_chars(LCD_SEG_L2_4_0, (u8 *)str, SEG_ON);

	TA1CCTL2 &= ~CCIFG;

	TA0CCR0 = rand;
	TA0CTL |= MC_1;	//	start TimerA0 in up mode
}
