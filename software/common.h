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

/** @file common.h
	@brief Plik nagłówkowy - Definicje wspólne dla wszystkich projektów.
*/

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#define GLUE(a, b)     a##b
#define PORT(x)        GLUE(PORT, x)
#define PIN(x)         GLUE(PIN, x)
#define DDR(x)         GLUE(DDR, x)
#define pin(x)         GLUE(pin, x)

/** Definicja prawdy.
*/
#define TRUE 1

/** Definicja fałszu.
*/
#define FALSE 0

/** Definicja włączenia.
*/
#define ON 1

/** Definicja wyłączenia.
*/
#define OFF 0

/** Definicja wartości zablokowanej / nieaktywnej dla zmiennych 16-bitowych.
*/
#define UINT16_T_DISABLED 0xFFFF
/** Definicja wartości zablokowanej / nieaktywnej dla zmiennych 8-bitowych.
*/
#define UINT8_T_DISABLED 0xFF
