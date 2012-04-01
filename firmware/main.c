/*
 * Copyright 2012 Fabio Baltieri (fabio.baltieri@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "board.h"

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "usbdrv.h"
#include "requests.h"

#include "clock.h"

/*
 * USB
 */

static void reset_cpu(void)
{
	wdt_disable();

	wdt_enable(WDTO_15MS);

	for (;;);
}

usbMsgLen_t usbFunctionSetup(uint8_t data[8])
{
	struct usbRequest *rq = (void *)data;
	static union {
		uint32_t l;
		uint8_t  b[4];
	} temp;

	switch (rq->bRequest) {
	case CUSTOM_RQ_GET_TIME:
		temp.l = time;
		usbMsgPtr = (uint8_t *)&temp;
		return sizeof(temp);
	case CUSTOM_RQ_SET_TIME:
		temp.b[0] = rq->wValue.bytes[0];
		temp.b[1] = rq->wValue.bytes[1];
		temp.b[2] = rq->wIndex.bytes[0];
		temp.b[3] = rq->wIndex.bytes[1];
		time = temp.l;
		return 0;
	case CUSTOM_RQ_RESET:
		reset_cpu();
		break;
	}

	return 0;
}

/*
 * Main & Friends
 */

static void hello(void)
{
	uint8_t i;
	drive_0(1);
	for (i = 0; i < 8; i++) {
		led_set(0xff);
		_delay_ms(33);
		led_set(0x00);
		_delay_ms(33);
	}
	drive_0(0);
}

int __attribute__((noreturn)) main(void)
{
	uint8_t i;

	/* disable watchdog */
	i = MCUSR;
	MCUSR = 0;
	wdt_disable();

	/* power down unused peripherals */
	PRR = ( (1 << PRTWI) |    /* TWI */
		(0 << PRTIM0) |   /* Timer 0 */
		(0 << PRTIM1) |   /* Timer 1 */
		(1 << PRTIM2) |   /* Timer 2 */
		(1 << PRSPI) |    /* SPI */
		(1 << PRUSART0) | /* USART0 */
		(0 << PRADC) );   /* ADC */

	/* clock */
	led_init();
	clock_init();
	hello();

	/* USB */
	usbInit();
	usbDeviceDisconnect();

	i = 0;
	while (--i) {
		_delay_ms(1);
	}

	usbDeviceConnect();

	sei();
	for (;;) {
		usbPoll();

		clock_poll();

		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_mode();
	}
}
