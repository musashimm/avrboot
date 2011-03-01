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

/** @file usart_lite.c
	@brief Obsługa interfejsu szeregowego.
*/

#include "usart_lite.h"

/** Inicjalizacja pracy interfejsu szeregowego UART.
	@param ubrr wrtość określająca szybkość transmisji,
*/
void usart_init(unsigned int ubrr) {
	UBRRH = (unsigned char)(ubrr>>8);
	UBRRL = (unsigned char)ubrr;
	UCSRB = _BV(RXEN)|_BV(TXEN)|_BV(RXCIE);
	UCSRC = _BV(URSEL)|_BV(UCSZ1)|_BV(UCSZ0);
}

/** Wysyła bajt danych.
	@param data bajto do wysłania,
*/
void usart_transmit( unsigned char data ) {
	while( !(UCSRA & _BV(UDRE)) );
	UDR = data;
}
