#include <avr/io.h>
#include <string.h>
#include <stdlib.h>
#include "serial.h"

uint8_t UART::last_buffer = 0;

UART::UART(unsigned long int baud)
{
  baudrate = baud;
}

void UART::setUART(void)
{
  int ubrr = ((F_CPU / 16 + baudrate / 2) / baudrate - 1);
  UBRR0H = (ubrr >> 8);
  UBRR0L = ubrr;

  UCSR0B = (1 << TXEN0)| (1 << TXCIE0) | (1 << RXEN0) | (1 << RXCIE0);   // Enable receiver and transmitter
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);    // Set frame: 8data, 1 stp
}

void UART::sendChar(char tosend)
{
  while (( UCSR0A & (1<<UDRE0))  == 0){};
  UDR0 = tosend;
}

void UART::print(const char *text)
{
  uint8_t i;
  for(i = 0; i < strlen(text); i++)
  {
    sendChar(text[i]);
  }
}

void UART::println(const char *text)
{
  print(text);
  print("\n");
}

void UART::write(uint8_t val)
{
  char buffer[4];
  itoa(val, buffer, 10);
  println(buffer);
}

void UART::write(uint16_t val)
{
  char buffer[6];
  itoa(val, buffer, 10);
  println(buffer);
}

void UART::write(uint32_t val)
{
  char buffer[11];
  ltoa(val, buffer, 10);
  println(buffer);
}

void UART::write(long int val)
{
  char buffer[11];
  ltoa(val, buffer, 10);
  println(buffer);
}

void UART::toBuffer(void)
{
  buffer[last_buffer] = getChar();
  last_buffer += 1;
}

char UART::read(void)
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

uint8_t UART::available(void)
{
  return last_buffer;
}

char UART::getChar(void)
{
  while (!(UCSR0A & _BV(RXC0)));
  return (char) UDR0;
}
