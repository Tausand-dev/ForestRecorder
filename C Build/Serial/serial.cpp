#include <avr/io.h>
#include <string.h>
#include <stdlib.h>
#include "serial.h"

UART::UART(unsigned long int baud)
{
  baudrate = baud;
  last_buffer = 0;
}

void UART::flush(void) volatile
{
  last_buffer = 0;
}

void UART::setUART(void) volatile
{
  int ubrr = ((F_CPU / 16 + baudrate / 2) / baudrate - 1);
  UBRR0H = (ubrr >> 8);
  UBRR0L = ubrr;

  UCSR0B = (1 << TXEN0)| (1 << TXCIE0) | (1 << RXEN0) | (1 << RXCIE0);   // Enable receiver and transmitter
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);    // Set frame: 8data, 1 stp
}

void UART::sendChar(char tosend) volatile
{
  while (( UCSR0A & (1<<UDRE0))  == 0){};
  UDR0 = tosend;
}

void UART::print(const char *text) volatile
{
  uint8_t i;
  for(i = 0; i < strlen(text); i++)
  {
    sendChar(text[i]);
  }
}

void UART::println (const char *text) volatile
{
  print(text);
  print("\n");
}

void UART::write(uint8_t val) volatile
{
  char buffer[4];
  itoa(val, buffer, 10);
  print(buffer);
}

void UART::write(uint16_t val) volatile
{
  char buffer[6];
  itoa(val, buffer, 10);
  print(buffer);
}

void UART::write(int val) volatile
{
  char buffer[6];
  itoa(val, buffer, 10);
  print(buffer);
}

void UART::write(uint32_t val) volatile
{
  char buffer[11];
  ltoa(val, buffer, 10);
  print(buffer);
}

void UART::write(long int val) volatile
{
  char buffer[11];
  ltoa(val, buffer, 10);
  print(buffer);
}

void UART::toBuffer(void) volatile
{
  buffer[last_buffer] = getChar();
  last_buffer += 1;
}

unsigned char UART::read(void) volatile
{
  uint8_t i;
  char temp = buffer[0];
  for(i = 0; i < last_buffer; i++)
  {
    buffer[i] = buffer[i+1];
  }

  if(last_buffer == 0)
  {
    return ' ';
  }
  else
  {
    last_buffer -= 1;
    return temp;
  }
}

uint8_t UART::available(void) volatile
{
  return last_buffer;
}

unsigned char UART::getChar(void) volatile
{
  while (!(UCSR0A & _BV(RXC0)));
  return (char) UDR0;
}
