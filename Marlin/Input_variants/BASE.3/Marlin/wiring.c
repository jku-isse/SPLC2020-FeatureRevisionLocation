/*
  wiring.c - Partial implementation of the Wiring API for the ATmega8.
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2005-2006 David A. Mellis

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA

  $Id: wiring.c 388 2008-03-08 22:05:23Z mellis $
*/

#include "wiring_private.h"

volatile unsigned long timer0_millis = 0;

SIGNAL(TIMER0_OVF_vect)
{
	// timer 0 prescale factor is 64 and the timer overflows at 256
        timer0_millis++;
}

unsigned long millis()
{
	unsigned long m;
	uint8_t oldSREG = SREG;
	
	// disable interrupts while we read timer0_millis or we might get an
	// inconsistent value (e.g. in the middle of the timer0_millis++)
	cli();
	m = timer0_millis;
	SREG = oldSREG;
	
	return m;
}

void delay(unsigned long ms)
{
	unsigned long start = millis();
	
	while (millis() - start <= ms)
		;
}

/* Delay for the given number of microseconds.  Assumes a 8 or 16 MHz clock. 
 * Disables interrupts, which will disrupt the millis() function if used
 * too frequently. */
void delayMicroseconds(unsigned int us)
{
	uint8_t oldSREG;

	// calling avrlib's delay_us() function with low values (e.g. 1 or
	// 2 microseconds) gives delays longer than desired.
	//delay_us(us);


	

	
	
	
		

	
	
	
	

	
	

	// for the 8 MHz internal clock on the ATmega168

	// for a one- or two-microsecond delay, simply return.  the overhead of
	// the function calls takes more than two microseconds.  can't just
	// subtract two, since us is unsigned; we'd overflow.
	if (--us == 0)
		return;
	if (--us == 0)
		return;

	// the following loop takes half of a microsecond (4 cycles)
	// per iteration, so execute it twice for each microsecond of
	// delay requested.
	us <<= 1;

	// partially compensate for the time taken by the preceeding commands.
	// we can't subtract any more than this or we'd overflow w/ small delays.
	us--;


	// disable interrupts, otherwise the timer 0 overflow interrupt that
	// tracks milliseconds will make us delay longer than we want.
	oldSREG = SREG;
	cli();

	// busy wait
	__asm__ __volatile__ (
		"1: sbiw %0,1" "\n\t" // 2 cycles
		"brne 1b" : "=w" (us) : "0" (us) // 2 cycles
	);

	// reenable interrupts.
	SREG = oldSREG;
}

void init()
{
	// this needs to be called before setup() or some functions won't
	// work there
	sei();
	
	// on the ATmega168, timer 0 is also used for fast hardware pwm
	// (using phase-correct PWM would mean that timer 0 overflowed half as often
	// resulting in different millis() behavior on the ATmega8 and ATmega168)
	sbi(TCCR0A, WGM01);
	sbi(TCCR0A, WGM00);

	// set timer 0 prescale factor to 64
	sbi(TCCR0B, CS01);
	sbi(TCCR0B, CS00);

	// enable timer 0 overflow interrupt
	sbi(TIMSK0, TOIE0);

	// timers 1 and 2 are used for phase-correct hardware pwm
	// this is better for motors as it ensures an even waveform
	// note, however, that fast pwm mode can achieve a frequency of up
	// 8 MHz (with a 16 MHz clock) at 50% duty cycle

	
	
	

	
	

	
	

	
	

	
	
	
	
	
	
	

	
	

	
	
	
	
	
	
	
	

}
