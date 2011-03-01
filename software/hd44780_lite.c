/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.        Joerg Wunsch
 * ----------------------------------------------------------------------------
 *
 * HD44780 LCD display driver (Light version)
 *
 * ----------------------------------------------------------------------------
 * I extended and cleanuped a little this lcd libary, but as wish of Joerg the
 * library still stay under "THE BEER-WARE LICENSE"
 *                                                              Wojciech Todryk
 * ----------------------------------------------------------------------------
 * $Id: hd44780.c 46 2009-03-19 21:19:39Z musashi@todryk.pl $
 */

/** @file hd44780_lite.c
	@brief Obsługa wyświetlacza LCD.
*/

#include "avrboot.h"

#include <stdbool.h>
#include <stdint.h>
#include <avr/pgmspace.h>

#include <avr/io.h>
#include <util/delay.h>

#include "hd44780_lite.h"

/*
 * Send one pulse to the E signal (enable).  Mind the timing
 * constraints.  If readback is set to true, read the HD44780 data
 * pins right before the falling edge of E, and return that value.
 */
static inline uint8_t
hd44780_pulse_e(bool readback) __attribute__((always_inline));

static inline uint8_t
hd44780_pulse_e(bool readback)
{
  uint8_t x;

  HD44780_CONTROL_PORTOUT |= _BV(HD44780_E);
  /*
   * Guarantee at least 500 ns of pulse width.  For high CPU
   * frequencies, a delay loop is used.  For lower frequencies, NOPs
   * are used, and at or below 1 MHz, the native pulse width will
   * already be 1 us or more so no additional delays are needed.
   */
#if F_CPU > 4000000UL
  _delay_us(1);
#else
  /*
   * When reading back, we need one additional NOP, as the value read
   * back from the input pin is sampled close to the beginning of a
   * CPU clock cycle, while the previous edge on the output pin is
   * generated towards the end of a CPU clock cycle.
   */
  if (readback)
    __asm__ volatile("nop");
#  if F_CPU > 1000000UL
  __asm__ volatile("nop");
#    if F_CPU > 2000000UL
  __asm__ volatile("nop");
  __asm__ volatile("nop");
#    endif /* F_CPU > 2000000UL */
#  endif /* F_CPU > 1000000UL */
#endif
  if (readback)
    x = HD44780_DATA_PORTIN & HD44780_DATABITS;
  else
    x = 0;
  HD44780_CONTROL_PORTOUT &= ~_BV(HD44780_E);

  return x;
}

/*
 * Send one nibble out to the LCD controller.
 */
static void
hd44780_outnibble(uint8_t n, uint8_t rs)
{
  uint8_t x;

  HD44780_CONTROL_PORTOUT &= ~_BV(HD44780_RW);
  if (rs)
    HD44780_CONTROL_PORTOUT |= _BV(HD44780_RS);
  else
    HD44780_CONTROL_PORTOUT &= ~_BV(HD44780_RS);
  x = (HD44780_DATA_PORTOUT & ~HD44780_DATABITS) | ((n << HD44780_D4) & HD44780_DATABITS);
  HD44780_DATA_PORTOUT = x;
  (void)hd44780_pulse_e(false);
}

/*
 * Send one byte to the LCD controller.  As we are in 4-bit mode, we
 * have to send two nibbles.
 */
void
hd44780_outbyte(uint8_t b, uint8_t rs)
{
  hd44780_outnibble(b >> 4, rs);
  hd44780_outnibble(b & 0xf, rs);
  hd44780_wait_ready();
}

/*
 * Read one nibble from the LCD controller.
 */
static uint8_t
hd44780_innibble(uint8_t rs)
{
  uint8_t x;

  HD44780_CONTROL_PORTOUT |= _BV(HD44780_RW);
  HD44780_DATA_DDR &= ~HD44780_DATABITS;
  if (rs)
    HD44780_CONTROL_PORTOUT |= _BV(HD44780_RS);
  else
    HD44780_CONTROL_PORTOUT &= ~_BV(HD44780_RS);
  x = hd44780_pulse_e(true);
  HD44780_DATA_DDR |= HD44780_DATABITS;
  HD44780_CONTROL_PORTOUT &= ~_BV(HD44780_RW);

  return (x & HD44780_DATABITS) >> HD44780_D4;
}

/*
 * Read one byte (i.e. two nibbles) from the LCD controller.
 */
uint8_t
hd44780_inbyte(uint8_t rs)
{
  uint8_t x;
  x = hd44780_innibble(rs) << 4;
  x |= hd44780_innibble(rs);
  return x;
}

/*
 * Wait until the busy flag is cleared.
 */
void
hd44780_wait_ready(void)
{
  while (hd44780_incmd() & HD44780_BUSYFLAG) ;
}

/*
 * Initialize the LCD controller.
 *
 * The initialization sequence has a mandatory timing so the
 * controller can safely recognize the type of interface desired.
 * This is the only area where timed waits are really needed as
 * the busy flag cannot be probed initially.
 */
void
hd44780_init(void)
{

  HD44780_CONTROL_DDR |= _BV(HD44780_RS) | _BV(HD44780_RW) | _BV(HD44780_E);
  HD44780_DATA_DDR |= HD44780_DATABITS;
  HD44780_BL_DDR |= _BV(HD44780_BL_SWITCH);

  _delay_ms(30);
  _delay_ms(30);
  _delay_ms(30);
  _delay_ms(10);		/* 40 ms needed for Vcc = 2.7 V */
  hd44780_outnibble(HD44780_FNSET(1, 0, 0) >> 4, 0);
  _delay_ms(30);
  _delay_ms(30);
  _delay_ms(30);
  _delay_ms(10);
  hd44780_outnibble(HD44780_FNSET(1, 0, 0) >> 4, 0);
  _delay_ms(30);
  _delay_ms(30);
  _delay_ms(30);
  _delay_ms(10);;
  hd44780_outnibble(HD44780_FNSET(1, 0, 0) >> 4, 0);

  hd44780_outnibble(HD44780_FNSET(0, 1, 0) >> 4, 0);
  hd44780_wait_ready();
  hd44780_outcmd(HD44780_FNSET(0, 1, 0));
  hd44780_wait_ready();
  hd44780_outcmd(HD44780_DISPCTL(0, 0, 0));
  hd44780_wait_ready();
}

void hd44780_goto(uint8_t line, uint8_t pos) {
	hd44780_outcmd(HD44780_DDADDR(((line-1)*0x40)+pos-1));
}

void hd44780_out4hex(const uint8_t i) {
	i > 9 ? hd44780_outdata((i)-10+0x41) : hd44780_outdata((i)+0x30);
}

void hd44780_out8hex(const uint8_t i) {
	hd44780_out4hex(i >> 4);
	hd44780_out4hex(i & 0x0f);
}

void hd44780_outstrn_P(const char *s)
{
	register char c;

	while ((c = pgm_read_byte(s++))) {
		if (c == '\n') {
			hd44780_goto(2,1);
			continue;
		}
		hd44780_outdata(c);
	}
}
