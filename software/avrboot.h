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

/** @file avrboot.h
	@brief Plik nagłówkowy - Główna pętla programowa.
*/

#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include "hd44780_lite.h"
#include "common.h"
#include "usart_lite.h"

#define PROGRAM_NAME "AvrBoot"
#define PROGRAM_VERSION_MAIN 1
#define PROGRAM_VERSION_MAJOR 0
#define PROGRAM_VERSION_MINOR 0
#define PROGRAM_FULL_NAME  "    " PROGRAM_NAME "\n  wersja " STRINGIFY(PROGRAM_VERSION_MAIN) "." STRINGIFY(PROGRAM_VERSION_MAJOR) "." STRINGIFY(PROGRAM_VERSION_MINOR)


/** Wrtość do funkcji generującej "duże" opóźnenia
*/
#define ONE_SECOND 50

/** Początek BLS. Wartość w bajtach!!!
*/
#define BOOTSTART	2 * 0x3C00

/** Wartość timeout'u w sekundach w przypadku nie odbierania danych
*/
#define NO_VALID_DATA_RECEIVED_TIMEOUT	10

/** Szybkość transmisji
*/
#define BAUDRATE 19200

/** Ustawienia początkowe timera */
#define TAU1 7813

/** Ustawienia dla protokołu XMODEM
Struktura
	- 128 bajty dane,
	- 3 bajty nagłowek,
	- 2 bajty 16-bitowe CRC.
*/
#define DATA_BLOCK_LENGTH 128
#define DATA_BLOCK_HEADER_LENGTH 3
#define DATA_BLOCK_CRC_LENGTH 2
#define XMODEM_BLOCK_LENGTH DATA_BLOCK_HEADER_LENGTH + DATA_BLOCK_LENGTH + DATA_BLOCK_CRC_LENGTH
#define XMODEM_CRC_LOW_BYTE DATA_BLOCK_HEADER_LENGTH + DATA_BLOCK_LENGTH
#define XMODEM_CRC_HIGH_BYTE XMODEM_CRC_LOW_BYTE + 1
#define ASCIIC_TRANSMIT_RETRY_INTERVAL 3

/** możliwe symbole protokołu XMODEM.
*/
enum XMODEM_SYMBOL {SOH=0x01,EOT=0x04,ACK=0x06,NAK=0x15,ETB=0x17,CAN=0x18,ASCIIC=0x43};

/** sterowanie podswietlaniem LCD
*/
#define HD44780_BL_PORT A
#define HD44780_BL_SWITCH PORT7
/** Określa jakim poziomem włączane jest podświetlenie wyswietlacza. Jeżeli jest zdefiniowane to poziomem niskim, jeżeli nie to wysokim.
*/
//#define HD44780_BL_ON_LOW YES

/** obsluga wyswietlacza LCD */
#define HD44780_DATA_PORT C
#define HD44780_CONTROL_PORT A
#define HD44780_RS PORT4
#define HD44780_RW PORT5
#define HD44780_E  PORT6
#define HD44780_D4 PORT4
#define HD44780_D5 PORT5
#define HD44780_D6 PORT6
#define HD44780_D7 PORT7

/** pin wymuszający wykonanie kodu booloadera */
#define BOOT_FORCE_PORT D
#define BOOT_FORCE_DDR DDR(BOOT_FORCE_PORT)
#define BOOT_FORCE_PORTOUT PORT(BOOT_FORCE_PORT)
#define BOOT_FORCE_PIN_PORT PIN(BOOT_FORCE_PORT)
#define BOOT_FORCE_PIN PORT3

void long_delay(uint8_t);
int calcrc(char*, int);
void boot_program_page (uint32_t, uint8_t*);

