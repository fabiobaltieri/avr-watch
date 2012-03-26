#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#include "board.h"

#include "adc.h"

#define TIME_1S_TOP 375
#define TIME_TIMEOUT 5
#define BATT_TIMEOUT 5

uint32_t time;

static uint16_t jiffies;
static uint32_t step_mark;

static enum clock_state {
	S_STANDBY,
	S_TIME,
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

	digit_count = (digit_count + 1) % 5;
}

void clear_display(void)
{
	memset(digits, 0, sizeof(digits));
}

void show_voltage(void)
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
}

void show_time(void)
{
	uint8_t hour;
	uint8_t minute;
	uint8_t second;

	second = time % 60;
	minute = time / 60 % 60;
	hour = time / 60 / 60 % 24;

	digits[0] = numbers[minute / 10 % 10];
	digits[1] = numbers[minute % 10];
	/* digits[0] = numbers[hour / 10 % 10]; */
	/* digits[1] = numbers[hour % 10]; */
	digits[2] = numbers[second / 10 % 10];
	digits[3] = numbers[second % 10];
	digits[4] = 0xff;
}

ISR(TIMER2_COMPA_vect)
{
	jiffies = (jiffies + 1) % TIME_1S_TOP;

	if (jiffies == 0)
		time++;

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
		   (1 << CS02) | (0 << CS01) | (0 << CS00) );

	/* Timer 2 - real time couter */
	OCR2A = (F_CPU / 256 / TIME_1S_TOP) - 1;

	TIMSK2 = ( (1 << OCIE2A) );

	TCCR2A = ( (1 << WGM21) | (0 << WGM20) );
	TCCR2B = ( (0 << WGM22) |
		   (1 << CS22) | (1 << CS21) | (0 << CS20) );
}

void clock_init(void)
{
	sw_init();
	chg_init();
	timer_init();

	time = 0;
	jiffies = 0;
	digit_count = 0;

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
			state = S_TIME;
			step_mark = time;
		} else if (sw_b_read()) {
			show_voltage();
			state = S_BATT;
			step_mark = time;
		}
		break;
	case S_TIME:
		if (time > step_mark + TIME_TIMEOUT)
			state = S_STANDBY;
		break;
	case S_BATT:
		if (time > step_mark + BATT_TIMEOUT)
			state = S_STANDBY;
		break;
	}
}
