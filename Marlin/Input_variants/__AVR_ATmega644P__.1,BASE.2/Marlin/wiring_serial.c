/*
  wiring_serial.c - serial functions.
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2005-2006 David A. Mellis
  Modified 29 January 2009, Marius Kintel for Sanguino - http://www.sanguino.cc/

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA

  $Id: wiring.c 248 2007-02-03 15:36:30Z mellis $
*/


#include "wiring_private.h"

// Define constants and variables for buffering incoming serial data.  We're
// using a ring buffer (I think), in which rx_buffer_head is the index of the
// location to which to write the next incoming character and rx_buffer_tail
// is the index of the location from which to read.
#define RX_BUFFER_SIZE 128
#define RX_BUFFER_MASK 0x7f


unsigned char rx_buffer[2][128];
int rx_buffer_head[2] = {0, 0};
int rx_buffer_tail[2] = {0, 0};







#define BEGIN_SERIAL(uart_, baud_) { ##UBRR##uart_H = ((F_CPU / 16 + baud / 2) / baud - 1) >> 8; ##UBRR##uart_L = ((F_CPU / 16 + baud / 2) / baud - 1); ##UCSR##uart_A = 0; ##UCSR##uart_B = 0; ##UCSR##uart_C = 0; sbi(##UCSR##uart_B, ##RXENuart_); sbi(##UCSR##uart_B, ##TXENuart_); sbi(##UCSR##uart_B, ##RXCIEuart_); ##UCSR##uart_C = _BV(##UCSZ##uart_1)|_BV(##UCSZ##uart_0); }



















void beginSerial(uint8_t uart, long baud)
{
  if (uart == 0) { UBRR0H = ((F_CPU / 16 + baud / 2) / baud - 1) >> 8; UBRR0L = ((F_CPU / 16 + baud / 2) / baud - 1); UCSR0A = 0; UCSR0B = 0; UCSR0C = 0; sbi(UCSR0B, RXEN0); sbi(UCSR0B, TXEN0); sbi(UCSR0B, RXCIE0); UCSR0C = _BV(UCSZ01)|_BV(UCSZ00); }

  else { UBRR1H = ((F_CPU / 16 + baud / 2) / baud - 1) >> 8; UBRR1L = ((F_CPU / 16 + baud / 2) / baud - 1); UCSR1A = 0; UCSR1B = 0; UCSR1C = 0; sbi(UCSR1B, RXEN1); sbi(UCSR1B, TXEN1); sbi(UCSR1B, RXCIE1); UCSR1C = _BV(UCSZ11)|_BV(UCSZ10); }

}

#define SERIAL_WRITE(uart_, c_) while (!(##UCSR##uart_A & (1 << ##UDREuart_))) ; ##UDRuart_ = c




void serialWrite(uint8_t uart, unsigned char c)
{
  if (uart == 0) {
    while (!(UCSR0A & (1 << UDRE0))) ; UDR0 = c;
  }

  else {
    while (!(UCSR1A & (1 << UDRE1))) ; UDR1 = c;
  }

}

int serialAvailable(uint8_t uart)
{
  return (128 + rx_buffer_head[uart] - rx_buffer_tail[uart]) & 0x7f;
}

int serialRead(uint8_t uart)
{
  // if the head isn't ahead of the tail, we don't have any characters
  if (rx_buffer_head[uart] == rx_buffer_tail[uart]) {
    return -1;
  } else {
    unsigned char c = rx_buffer[uart][rx_buffer_tail[uart]];
    rx_buffer_tail[uart] = (rx_buffer_tail[uart] + 1) & 0x7f;
    return c;
  }
}

void serialFlush(uint8_t uart)
{
  // don't reverse this or there may be problems if the RX interrupt
  // occurs after reading the value of rx_buffer_head but before writing
  // the value to rx_buffer_tail; the previous value of rx_buffer_head
  // may be written to rx_buffer_tail, making it appear as if the buffer
  // were full, not empty.
  rx_buffer_head[uart] = rx_buffer_tail[uart];
}

#define UART_ISR(uart_) ISR(##USART##uart__RX_vect) { unsigned char c = ##UDRuart_; int i = (rx_buffer_head[uart_] + 1) & RX_BUFFER_MASK; if (i != rx_buffer_tail[uart_]) { rx_buffer[uart_][rx_buffer_head[uart_]] = c; rx_buffer_head[uart_] = i; } }
















ISR(USART0_RX_vect) { unsigned char c = UDR0; int i = (rx_buffer_head[0] + 1) & 0x7f; if (i != rx_buffer_tail[0]) { rx_buffer[0][rx_buffer_head[0]] = c; rx_buffer_head[0] = i; } }

ISR(USART1_RX_vect) { unsigned char c = UDR1; int i = (rx_buffer_head[1] + 1) & 0x7f; if (i != rx_buffer_tail[1]) { rx_buffer[1][rx_buffer_head[1]] = c; rx_buffer_head[1] = i; } }

