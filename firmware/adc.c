/*
 * Copyright 2011 Fabio Baltieri (fabio.baltieri@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "board.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "adc.h"

void adc_init(void)
{
	ADCSRA = ( (1 << ADEN)  | /* enable           */
		   (0 << ADSC)  | /* start conversion */
		   (0 << ADATE) | /* free running     */
		   (1 << ADIF)  | /* clear interrupts */
		   (1 << ADIE)  | /* interrupt enable */
		   (1 << ADPS2) | (1 << ADPS1) | (0 << ADPS0) );

	/* 12Mhz / 65 = 188kHz */
}

void adc_stop(void)
{
	ADMUX = 0x00;
	ADCSRA = 0x00;
}

ISR(ADC_vect)
{
	/* empty */
}

uint16_t adc_get (uint8_t cfg)
{
	ADMUX = cfg;

	/* wait Vref to stabilize */
	_delay_ms(5);

	set_sleep_mode(SLEEP_MODE_ADC);

	sleep_mode();

	loop_until_bit_is_clear(ADCSRA, ADSC);

	return ADCW;
}
