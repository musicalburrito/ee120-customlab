/*
 * final_project.c
 *
 * Created: 8/24/2017 7:15:41 PM
 *  Author: Nancy
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "io.c"

volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	//OCR1A = 37.5;
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
	TimerISR(); // Call the ISR that the user uses
	_avr_timer_cntcurr = _avr_timer_M;
}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

unsigned char score;
unsigned int pattern[9];
unsigned char level;
unsigned char counter;
unsigned int period;
unsigned int x = 58;
enum states {init, iterate, wait1, input, wrong, fall1, fall2, fall3, fall4, msg1, msg2, msg3, msg4} curr_state;
	
/*
https://en.wikipedia.org/wiki/Xorshift
*/
unsigned int xorshift32()
{
	x ^= x << 7;
	x ^= x >> 5;
	x ^= x << 3;
	return x;
}	
	
void start(){
	switch(curr_state){
		case msg1:
			LCD_DisplayString(1, "Welcome. Press a button to begin.");
			curr_state = init;
		case init:
			PORTB = 0x0F;
			period = 0;
			score = 0;
			level = 1;
			counter = 0;
			for(int i = 0; i < 8; ++i){
				pattern[i] = xorshift32(x) % 4;
			}
			if(PINA < 0xFF){
				curr_state = iterate;
			}
		break;
		case iterate:
			if (pattern[counter] == 0){
				PORTB = 0x01;
			}
			else if (pattern[counter] == 1){
				PORTB = 0x02;	
			}				
			else if (pattern[counter] == 2){
				PORTB = 0x04;
			}
			else if (pattern[counter] == 3){
				PORTB = 0x08;
			}	
			if(period == 20){
				curr_state = wait1;
				period = 0;
			}			
			++period;
		break;
		case wait1:
			PORTB = 0x00;
			if(period == 20){
				period = 0;
				++counter;
				if(counter < level){
					curr_state = iterate;
				}
				else { 
					curr_state = msg2; 
					counter = 0;
				}
			} else { ++period; }							
		break;
		case msg2:
			LCD_DisplayString(1, "Input");
			curr_state = input;
		break;
		case input:
			if(counter == level){
				if(level < 3){
					++level;
					LCD_DisplayString(1, "Level: ");
					LCD_Cursor(8);
					LCD_WriteData(level + '0');
					counter = 0;
					curr_state = iterate;
				} else {
					LCD_DisplayString(1, "You win!");
					curr_state = msg4;
				}
			}
			if(~PINA & 0x01){
				if(pattern[counter] == 0){
					LCD_DisplayString(1, "1");
					++counter;
					curr_state = fall1;
				} else { curr_state = msg3; }
			}
			else if(~PINA & 0x02){
				if(pattern[counter] == 1){
					LCD_DisplayString(1, "2");
					++counter;
					curr_state = fall2;
				} else { curr_state = msg3; }
			}
			else if(~PINA & 0x04){
				if(pattern[counter] == 2){
					LCD_DisplayString(1, "3");
					++counter;
					curr_state = fall3;
				} else { curr_state = msg3; }
			}
			else if(~PINA & 0x08){
				if(pattern[counter] == 3){
					LCD_DisplayString(1, "4");
					++counter;
					curr_state = fall4;
				} else { curr_state = msg3; }
			}
		break;
		case msg3:
			LCD_DisplayString(1, "You lose.");
			curr_state = wrong;
		break;
		case wrong:
			period++;
			if(period == 40){
				curr_state = msg1;
			}			
		break;
		case fall1:
			if(~(~PINA & 0x01)){
				curr_state = msg2;
			}
		break;
		case fall2:
			if(~(~PINA & 0x02)){
				curr_state = msg2;
			}
		break;
		case fall3:
			if(~(~PINA & 0x04)){
				curr_state = msg2;
			}
		break;
		case fall4:
			if(~(~PINA & 0x08)){
				curr_state = msg2;
			}
		break;
		case msg4:
			if(period == 0){
				LCD_DisplayString(1, "You win.");
			}			
			++period;
			if(period == 10){				
				curr_state = msg1;
				period = 0;
			}			
		break;
	}
}

int main(void)
{
	DDRC = 0xFF; PORTC = 0x00; // LCD data lines
	DDRD = 0xFF; PORTD = 0x00; // LCD control lines
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	LCD_init();
	TimerSet(50);
	TimerOn();
	curr_state = msg1;
    while(1)
    {
		while(!TimerFlag);
		TimerFlag = 0;
		start();
        //TODO:: Please write your application code 
    }
}