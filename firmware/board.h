/* LEDs */

#define SSEG_A	PC3
#define SSEG_B	PC2
#define SSEG_D	PC0
#define SSEG_E	PC1
#define SSEG_F	PC4
#define SSEG_G	PC5
#define SSEG_C	PB5
#define SSEG_H	PB4

#define SSEG_PORT_D01	PORTD
#define SSEG_DDR_D01	DDRD
#define SSEG_PORT_D234	PORTB
#define SSEG_DDR_D234	DDRB

#define SSEG_D0	PD5 /* OC0B */
#define SSEG_D1	PD6 /* OC0A */
#define SSEG_D2	PB2 /* OC1B */
#define SSEG_D3	PB3 /* OC2A */
#define SSEG_D4	PB1 /* OC1A */

#define drive_0(x) do {					\
		if (x)					\
			SSEG_DDR_D01 |= _BV(SSEG_D0);	\
		else					\
			SSEG_DDR_D01 &= ~_BV(SSEG_D0);	\
	} while (0)
#define drive_1(x) do {					\
		if (x)					\
			SSEG_DDR_D01 |= _BV(SSEG_D1);	\
		else					\
			SSEG_DDR_D01 &= ~_BV(SSEG_D1);	\
	} while (0)
#define drive_2(x) do {					\
		if (x)					\
			SSEG_DDR_D234  |= _BV(SSEG_D2);	\
		else					\
			SSEG_DDR_D234 &= ~_BV(SSEG_D2);	\
	} while (0)
#define drive_3(x) do {					\
		if (x)					\
			SSEG_DDR_D234  |= _BV(SSEG_D3);	\
		else					\
			SSEG_DDR_D234 &= ~_BV(SSEG_D3);	\
	} while (0)
#define drive_4(x) do {					\
		if (x)					\
			SSEG_DDR_D234  |= _BV(SSEG_D4);	\
		else					\
			SSEG_DDR_D234 &= ~_BV(SSEG_D4);	\
	} while (0)

#define led_init() do {				\
		DDRC |= 0x3f;			\
		DDRB |= 0x30;			\
	} while (0)

#define led_set(x) do {						\
		PORTC = (PORTC & ~0x3f) | (x & 0x3f);		\
		PORTB = (PORTB & ~0x30) | ((x & 0xc0) >> 2);	\
	} while (0)

/* Buttons */

#define SW_A_PCINT	PCINT0
#define SW_A_PCMSK	PCMSK0
#define SW_A_PORT	PORTB
#define SW_A_PIN	PINB
#define SW_A		PB0

#define SW_B_PCINT	PCINT23
#define SW_B_PCMSK	PCMSK2
#define SW_B_PORT	PORTD
#define SW_B_PIN	PIND
#define SW_B		PD7

#define sw_init() do {				\
		SW_A_PORT |= _BV(SW_A);		\
		SW_B_PORT |= _BV(SW_B);		\
		SW_A_PCMSK |= _BV(SW_A_PCINT);	\
		SW_B_PCMSK |= _BV(SW_B_PCINT);	\
	} while (0)

#define sw_a_read() (!(SW_A_PIN & _BV(SW_A)))
#define sw_b_read() (!(SW_B_PIN & _BV(SW_B)))

/* CHG */
#define CHG_PCINT	PCINT20
#define CHG_PCMSK	PCMSK2
#define CHG_PORT	PORTD
#define CHG_PIN		PIND
#define CHG		PD4

#define chg_init() do {				\
		CHG_PORT |= _BV(CHG);		\
		CHG_PCMSK |= _BV(CHG_PCINT);	\
	} while (0)

#define chg_read() (!(CHG_PIN & _BV(CHG)))

/* Analog */

#define AIN_BATTERY	(_BV(REFS0) | 0x0e) /* measure Vbg, AVcc reference */
