/*
       ___   _____   ___         _
      /_\ \ / / _ \ | _ )___ ___| |_
     / _ \ V /|   / | _ | _ | _ \  _|
    /_/ \_\_/ |_|_\ |___|___|___/\__|

    AvrBoot - LCD Bootloader for AVR microcontrollers
    Copyright (C) 2009 Wojciech Todryk (wojciech@todryk.pl)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	$Id:$
*/

/** @file avrboot.c
	@brief Główna pętla programowa
*/

#include "avrboot.h"

/** Stan komunikacji między nadawcą i odbiorcą.
*/
enum STATUS {	OK, 			/**< Odebrano wszystkie bloki danych poprawnie.*/
				TOO_SMALL_MEM, 	/**< Liczba odebranych bloków jest większa niż dostępna pamięć w mikrokontrolerze.*/				NO_VALID_DATA_RECEIVED /**< Odebrano blok z błędem CRC lub nieodebrano żadnego bloku (przerwa w transmisji).*/
			} status;

/** Adres strony pamięci do zaprogramowania.
*/
uint32_t flash_address;

/** Index bufora z danymi.
*/
volatile uint8_t pdata;

/** Licznik sekund.
*/
volatile uint8_t seconds;

/** Licznik odbieranych bloków.
*/
volatile uint8_t block_num;

/** Bufor z danymi.
*/
uint8_t data[XMODEM_BLOCK_LENGTH];

/** Inicjalizacja urządzeń wejścia / wyjścia.
*/
void ioinit(void) {

	hd44780_init();
	hd44780_clear();
	hd44780_bl_on();
	hd44780_cursor_off();

	BOOT_FORCE_PORTOUT |= _BV(BOOT_FORCE_PIN);
	BOOT_FORCE_DDR &= ~_BV(BOOT_FORCE_PIN);

	usart_init(UBRR_VALUE);

	TCCR1B = _BV(WGM12) | _BV(CS10) | _BV(CS12);            // licznik / 1024 - wyzerwoanie na porownanie
	TIMSK = _BV(OCIE1A);                                    // przerwanie na porownanie wartosci
	OCR1A = TAU1;                                           // wartość licznika porownania
															// przerwanie co 1 sekunde
}

/** Przerwanie licznika.
*/
ISR (TIMER1_COMPA_vect) {
	seconds++;
}

/** Przerwanie po odebraniu znaku z UARTa.
*/
ISR (USART_RXC_vect) {
    data[pdata++] = UDR;
}

/** Główna funkcja programu.
*/
int main(void) {

	int crc;

	ioinit();

	hd44780_outstrn_P(PSTR(PROGRAM_FULL_NAME));
	long_delay(ONE_SECOND);
	hd44780_clear();

	if (BOOT_FORCE_PIN_PORT & _BV(BOOT_FORCE_PIN)) {
		hd44780_outstrn_P(PSTR("    Reset..."));
		long_delay(ONE_SECOND);
        asm("jmp 0000");
	}

	wdt_enable(WDTO_2S);

	hd44780_outstrn_P(PSTR("    Waiting\n   for data..."));

	GICR = _BV(IVCE);							            // przeniesienie tablicy przerwań
	GICR = _BV(IVSEL);
	sei();                                                  // włączenie przerwań

	usart_transmit(ASCIIC);

	while (pdata == 0) {                                    // oczekiwanie na dane w buforze
	    wdt_reset();
		if (seconds >= ASCIIC_TRANSMIT_RETRY_INTERVAL) {    // czas minął, wysyłamy "C" jeszcze raz
			usart_transmit(ASCIIC);
			seconds = 0;
		}
	}

	hd44780_clear();

	for (;;) {

	    if (seconds >= NO_VALID_DATA_RECEIVED_TIMEOUT) {    // nie otrzymujemy poprawnych danych
            status = NO_VALID_DATA_RECEIVED;
            break;
	    }

		if (pdata == 1) {
			if (data[0] == EOT) {							// otrzymano symbol zakończenia nadawania
				usart_transmit(ACK);
				break;
			}
		} else if (pdata >= XMODEM_BLOCK_LENGTH ) {         // pełny bufor, zaczynamy weryfikację danych

		    if (flash_address >= BOOTSTART) {               // program, który chcemy załadować jest większy niż dostępna pamięć
                status = TOO_SMALL_MEM;
                break;
            }

			hd44780_home();
			block_num++;
			hd44780_outstrn_P(PSTR("  Block\n  received: "));
			hd44780_out8hex(block_num);
			pdata = 0;
			crc = calcrc((char*)data+DATA_BLOCK_HEADER_LENGTH, DATA_BLOCK_LENGTH);

			if ((data[XMODEM_CRC_LOW_BYTE] == ((crc & 0xFF00) >> 8)) && (data[XMODEM_CRC_HIGH_BYTE] == (crc & 0xFF))) {
				boot_program_page (flash_address,data+DATA_BLOCK_HEADER_LENGTH);
				flash_address += SPM_PAGESIZE;
				seconds = 0;
                usart_transmit(ACK);
			} else {
				usart_transmit(NAK);
			}
		}
		wdt_reset();
	}

    hd44780_clear();
    switch (status) {
        case TOO_SMALL_MEM:
            hd44780_outstrn_P(PSTR("   Error:\n   Low mem"));
            wdt_disable();
            for(;;);
            break;
        case NO_VALID_DATA_RECEIVED:
            hd44780_outstrn_P(PSTR(" Error:\n No data / CRC "));
            wdt_disable();
            for(;;);
            break;
        default:
            cli();
            hd44780_outstrn_P(PSTR("   Done.\n   Reset..."));
            for(;;);
            break;
    }

  	return 0;
}

/** Wytwarza "duże" opóźnienia.
	@param count licznik wykonań pętli
*/
void long_delay(uint8_t count) {

	uint8_t i;

	for (i=0;i<count;i++) {
		_delay_ms(20);
	}
}

/** Oblicza funkcję CRC16.
	@param *ptr wskaźnik na znak w buforze,
	@param count liczba znaków w buforze
*/
int calcrc(char *ptr, int count)
{
    int  crc;
    char i;

    crc = 0;
    while (--count >= 0)
    {
        crc = crc ^ (int) *ptr++ << 8;
        i = 8;
        do
        {
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        } while(--i);
    }
    return (crc);
}

/** @brief Funkcja programuje jedną stronę w pamięci FLASH. Procedura żywcem wzięta z dokumentacji AVR-LIBC.
	@param page adres strony
	@param *buf wskaźnik buforu z danymi
*/
void boot_program_page (uint32_t page, uint8_t *buf) {
	uint16_t i;
    uint8_t sreg;

    // Disable interrupts.

    sreg = SREG;
    cli();

    eeprom_busy_wait ();

    boot_page_erase (page);
    boot_spm_busy_wait ();      // Wait until the memory is erased.

    for (i=0; i<SPM_PAGESIZE; i+=2) {
		// Set up little-endian word.

		uint16_t w = *buf++;
		w += (*buf++) << 8;

		boot_page_fill (page + i, w);
	}

	boot_page_write (page);     // Store buffer in flash page.
    boot_spm_busy_wait();       // Wait until the memory is written.

    // Reenable RWW-section again. We need this if we want to jump back
    // to the application after bootloading.

    boot_rww_enable ();

    // Re-enable interrupts (if they were ever enabled).

    SREG = sreg;
}

/** \mainpage Dokumentacja AvrBoot
	@section intro_sec Wstęp
	LCD Bootloader służy do programowania pamięci FLASH mikrokontrolera przy użyciu interfejsu innego niż ISP. W tej wersji obsługiwany jest tylko port UART. W bootloadrze zaimplementowany jest protokół <a href="http://www.amulettechnologies.com/support/help/xmodem.htm">XMODEM-CRC</a>. W protokole nadawca wysyła 128-bajtowe bloki danych poprzedzone nagłówkiem i zabezpieczone sumą CRC-16. Odbiorca po odebraniu bloku i sprawdzeniu jego poprawności odsyła sygnał ACK.

	@section features Cechy bootloadera
- wielkość kodu 1652 bajtów,
- komunikaty wyświetlane są na wyświetlaczu LCD,
- zaimplementowany protokół XMODEM, dzięki temu mikrokontroler może być programowany z wykorzystaniem np. MINICOMa (Linux) lub HyperTerminala (Windows),
- zaimplementowane zabezpieczenie przed podstawowaymi błędami,
- wykonanie kodu bootloadera wymuszone zmianą stanu na pinie mikrokontrolera.

*/


