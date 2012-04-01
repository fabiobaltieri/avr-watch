/*
 * Copyright 2012 Fabio Baltieri (fabio.baltieri@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#include "usbdrv.h"

#include "board.h"

#include "adc.h"

#define TIME_TIMEOUT 5
#define BATT_TIMEOUT 2
#define TICK_COUNT 150

uint32_t time;

static uint16_t ticks;
static uint8_t countdown;

static volatile enum clock_state {
	S_STANDBY,
	S_TIME,
	S_BATT_DEBOUNCE,
	S_BATT,
} state;

/*
 *   08
 * 10  04
 *   20
 * 02  80
 *   01  40
 */

static uint8_t digit_count;
static uint8_t digits[5];
static uint8_t numbers[] = { 0x9f, 0x84, 0x2f, 0xad, /* 0 1 2 3 */
			     0xb4, 0xb9, 0xbb, 0x8c, /* 4 5 6 7 */
			     0xbf, 0xbd, 0xbe, 0xb3, /* 8 9 A b */
			     0x1b, 0xa7, 0x3b, 0x3a, /* C d E F */
			     0x40, 0xa3, 0x20, 0x33, /* . o - t */
};
#define LCD_DOT 0x40

static int refresh_running;

static uint8_t usb_enabled;

#define CS0_MASK (_BV(CS02) | _BV(CS01) | _BV(CS00))
#define CS0_FAST ((0 << CS02) | (1 << CS01) | (1 << CS00))
#define CS0_SLOW ((1 << CS02) | (0 << CS01) | (0 << CS00))

#define CS1_MASK (_BV(CS12) | _BV(CS11) | _BV(CS10))
#define CS1_FAST ((0 << CS12) | (1 << CS11) | (1 << CS10))
#define CS1_SLOW ((1 << CS12) | (0 << CS11) | (0 << CS10))

static void usb_toggle(void)
{
	if (usb_enabled) {
		EIMSK &= ~_BV(INT0);
		usb_enabled = 0;

		/* slow down */

		CLKPR = _BV(CLKPCE);
		CLKPR = 2;

		TCCR1B = (TCCR1B & ~CS1_MASK) | CS1_FAST;
		TCCR0B = (TCCR0B & ~CS0_MASK) | CS0_FAST;
	} else {
		EIMSK |= _BV(INT0);
		usb_enabled = 1;

		/* speed up */

		TCCR0B = (TCCR0B & ~CS0_MASK) | CS0_SLOW;
		TCCR1B = (TCCR1B & ~CS1_MASK) | CS1_SLOW;

		CLKPR = _BV(CLKPCE);
		CLKPR = 0;
	}
}

ISR(TIMER0_COMPA_vect)
{
	led_set(0x00);

	switch (digit_count) {
	case 0:
		drive_4(0);
		drive_0(1);
		break;
	case 1:
		drive_0(0);
		drive_1(1);
		break;
	case 2:
		drive_1(0);
		drive_2(1);
		break;
	case 3:
		drive_2(0);
		drive_3(1);
		break;
	case 4:
		drive_3(0);
		drive_4(1);
		break;
	}

	led_set(digits[digit_count]);

	/* blink dots in TIME state */
	if (state == S_TIME) {
		if (ticks++ < TICK_COUNT)
			digits[4] = 0xff;
		else
			digits[4] = 0x00;
	}

	/* stop and clear on 4th digit */
	if (!refresh_running && digit_count == 4) {
		led_set(0x00);
		PRR |= _BV(PRTIM0);
	}

	digit_count = (digit_count + 1) % 5;
}

static void clear_display(void)
{
	memset(digits, 0, sizeof(digits));
}

static void show_voltage(void)
{
	uint32_t voltage;

	adc_init();
	voltage = adc_get(AIN_BATTERY);
	adc_stop();

	voltage = voltage * 1100 / 1024;
	voltage = voltage * (10 + 33) / 10;

	digits[0] = numbers[voltage / 10 / 10 / 10 % 10] | LCD_DOT;
	digits[1] = numbers[voltage / 10 / 10 % 10];
	digits[2] = numbers[voltage / 10 % 10];
	digits[3] = numbers[voltage % 10];
	if (usb_enabled)
		digits[3] |= LCD_DOT;
}

static void show_time(void)
{
	uint8_t hour;
	uint8_t minute;
	uint8_t second;

	second = time % 60;
	minute = time / 60 % 60;
	hour = time / 60 / 60 % 24;

	digits[0] = numbers[hour / 10 % 10];
	digits[1] = numbers[hour % 10];
	digits[2] = numbers[minute / 10 % 10];
	digits[3] = numbers[minute % 10];
}

ISR(TIMER1_COMPA_vect)
{
	time++;

	ticks = 0;

	if (countdown)
		countdown--;

	if (state == S_TIME)
		show_time();
}

ISR(PCINT0_vect)
{
}

ISR(PCINT1_vect)
{
}

ISR(PCINT2_vect)
{
}

static void timer_init(void)
{
	/* Timer 0 - display scan */

	OCR0A = (F_CPU / 256 / 5 / 200) - 1; /* 200Hz */

	TIMSK0 = ( (1 << OCIE0A) );

	TCCR0A = ( (1 << WGM01) | (0 << WGM00) ); /* CTC */
	TCCR0B = ( (0 << WGM02) |
		   CS0_SLOW );

	/* Timer 1 - real time couter */

	OCR1A = (F_CPU / 256) - 1;

	TIMSK1 = ( (1 << OCIE1A) );

	TCCR1A = ( (0 << WGM11) | (0 << WGM10) );
	TCCR1B = ( (0 << WGM13) | (1 << WGM12) |
		   CS1_SLOW );

	/* NOTE: TCCR*B are also modified in usb_toggle */
}

void clock_init(void)
{
	sw_init();
	chg_init();
	timer_init();

	time = 0;
	ticks = 0;
	digit_count = 0;
	refresh_running = 0;
	usb_enabled = 1;

	state = S_STANDBY;

	/* enable PC irqs */
	PCICR = _BV(PCIE0) | _BV(PCIE1) | _BV(PCIE2);
}

void clock_poll(void)
{
	switch (state) {
	case S_STANDBY:
		clear_display();
		if (sw_a_read()) {
			ticks = TICK_COUNT;
			show_time();
			state = S_TIME;
			countdown = TIME_TIMEOUT;
		} else if (sw_b_read()) {
			show_voltage();
			state = S_BATT_DEBOUNCE;
			countdown = BATT_TIMEOUT;
		}
		break;
	case S_TIME:
		if (countdown == 0)
			state = S_STANDBY;
		break;
	case S_BATT_DEBOUNCE:
		if (!sw_b_read())
			state = S_BATT;
		break;
	case S_BATT:
		if (sw_b_read()) {
			usb_toggle();
			show_voltage();
			state = S_BATT_DEBOUNCE;
			countdown = BATT_TIMEOUT;
		}
		if (countdown == 0)
			state = S_STANDBY;
		break;
	}

	if (state != S_STANDBY || chg_read()) {
		refresh_running = 1;
		PRR &= ~_BV(PRTIM0); /* start refresh timer */
	} else {
		refresh_running = 0;
	}

	/* battery charging indicator */
	if (state == S_STANDBY && chg_read()) {
		switch (time % 5) {
		case 4:
			digits[3] = LCD_DOT;
		case 3:
			digits[2] = LCD_DOT;
		case 2:
			digits[1] = LCD_DOT;
		case 1:
			digits[0] = LCD_DOT;
		}
	}
}
