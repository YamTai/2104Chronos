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

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    lcd_init();	//	LCD init

    /*initialise buttons*/
    P2DIR &= ~0x1F;	//	P2.0 ~ P2.4 as OP
    P2REN |= 0x1F;	//	P2.0 ~ P2.4 enable resistor
    P2OUT &= ~0x1F;	//	P2.0 ~ P2.4 pull down
    P2IES &= ~0x1F;	//	P2.0 ~ P2.4 interrupt edge select, Rising
    P2IFG &= ~0x1F;	//	P2.0 ~ P2.4 interrupt flag clear
    P2IE |= 0x1F;	//	P2.0 ~ P2.4 interrupt enable

    P2SEL |= 0x08;	//	P2.3 alternate function, TA1 CCR2 CCI2A

    TA0CTL |= TASSEL_1 | ID_3 | TACLR;	// ACLK (32768Hz) | /8 | timerA clear, 4096Hz (2^16/4096), MAX 16 seconds

	TA1CTL |= TASSEL_2 | MC_2 | TACLR;	//	SMCLK (?MHz) | Continuous mode | timer A clear
	TA1CCTL2 |= CM_1 | CCIS_0 | SCS | CAP | CCIE;	//	rising edge | input select CCI2A | Synchronous | Capture mode | Interrupt enable

    while(1){

        _BIS_SR(LPM3_bits + GIE);	// low power mode to save more powar
        __no_operation();

	}
}

#pragma vector = PORT2_VECTOR
__interrupt void Port_2(void){

	u8 *str;
	u16 temp;

	//	mgLSB are mg values per LSB, mutiplied by 100

	if (P2IFG == BUTTON_STAR_PIN){
		display_chars(LCD_SEG_L1_3_0, "STAR", SEG_ON);
	}else
	if (P2IFG == BUTTON_NUM_PIN){
		display_chars(LCD_SEG_L1_3_0, "MODE", SEG_ON);
	}else
	if (P2IFG == BUTTON_UP_PIN){
		display_chars(LCD_SEG_L1_3_0, "  UP", SEG_ON);
	}else
	if (P2IFG == BUTTON_DOWN_PIN){
		display_chars(LCD_SEG_L1_3_0, "  DN", SEG_ON);
	}else
	if (P2IFG == BUTTON_BACKLIGHT_PIN){
		display_chars(LCD_SEG_L1_3_0, "  BL", SEG_ON);
	}else
	if (P2IFG == Accel_int1){

		accel_get();

		display_chars(LCD_SEG_L1_3_0, "HI-G", SEG_ON);

		temp = remove_sign(xyz[0]) * mgLSB;

		str = int_to_array(temp, 5, 1);
		display_chars(LCD_SEG_L2_3_0, (u8 *)str, SEG_ON);
		display_symbol(LCD_SEG_L2_COL0, SEG_ON);

		if (is_neg(xyz[0])){
			display_char(LCD_SEG_L2_4, (u8)'L', SEG_ON);
		}else{
			display_char(LCD_SEG_L2_4, (u8)'R', SEG_ON);
		}

		TA0CCTL0 &= ~CCIE;	//	timerA0 compare/control register 0 interrupt disable
		TA0CCTL1 &= ~CCIE;	//	timerA0 compare/control register 1 interrupt disable

		TA0CTL |= MC_0 | TACLR;	//	stop and clear TimerA0
	}

	P2IFG &= ~0x1F;	// P2.0 ~ P2.4 interrupt flag clear
	P2IFG &= ~0x20;	// accel int1 clear

}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER_A5_CCR0_ISR(void){

//	TA0CCTL0 &= ~CCIFG;	//	TA0CCR0 interrupt flag auto clears

	accel_stop();

	display_chars(LCD_SEG_L2_4_0, (u8 *)"OOOOO", SEG_ON);

    TA0CCTL0 &= ~CCIE;	//	timerA0 compare/control register 0 interrupt disable
    TA0CCTL1 &= ~CCIE;	//	timerA0 compare/control register 1 interrupt disable

	TA0CTL |= MC_0 | TACLR;	//	stop and clear TimerA0
}


#pragma vector=TIMER0_A1_VECTOR
__interrupt void TIMER_A5_CCR1_ISR(void){

	if ((TA0CCTL1 & CCIFG) == 1){	//	TA0CCR1 interrupt

		accel_start();

		display_chars(LCD_SEG_L2_4_0, (u8 *)"-----", SEG_ON);

		TA0CCTL1 &= ~CCIFG;	//	clear TA0CCR1 interrupt flag
	}
}

#pragma vector=TIMER1_A1_VECTOR
__interrupt void TIMER_A3_CCR2_ISR(void){

	u8 *str;
	u16 rand;

	if ((TA1CCTL2 & CCIFG) == 1){	//	P2.3 interrupt (CCI2A)

		while((P2IN & BUTTON_BACKLIGHT_PIN) == BUTTON_BACKLIGHT_PIN);

		TA1CCTL2 &= ~CCIFG;	// clear TA1CCTL2 interrupt

		rand = TA1CCR2;
		rand ^= rand << 8;

		rand %= 0xA000;	// max of 10 seconds (4096 cycles *10)

		if (rand < 0x3000){	// min of 3 seconds (4096 *3)
			rand |= 0x3000;
		}

		str = int_to_array((rand/4096), 5, 0);	//	approximate delay
		display_chars(LCD_SEG_L2_4_0, (u8 *)str, SEG_ON);
		clear_line(1);
		display_symbol(LCD_SEG_L2_COL0, SEG_OFF);

	    TA0CCTL0 |= CCIE;	//	timerA0 compare/control register 0 interrupt enable
	    TA0CCTL1 |= CCIE;	//	timerA0 compare/control register 1 interrupt enable

		TA0CCR0 = rand + 0x1000;	//	set TA0CCR0 to 'random' value + offset of .5 seconds(ACLK/8)
		TA0CCR1 = rand;				//	set TA0CCR1 to 'random' value
		TA0CTL |= MC_1;				//	start TimerA0 in up mode
	}
}
