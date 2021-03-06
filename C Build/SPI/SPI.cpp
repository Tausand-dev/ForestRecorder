#include <avr/io.h>
#include <stdint.h>
#include "SPI.h"

uint8_t digitalRead(volatile uint8_t *pin, uint8_t n)
{
  return *pin & (1 << n);
}

void digitalWrite(volatile uint8_t *port, uint8_t n, uint8_t val)
{
  if(val)
  {
    *port |= (1 << n);
  }
  else
  {
    *port &= ~(1 << n);
  }
}

SPI::SPI(void)
{
  speed = 0;
}

void SPI::setSpeed(uint8_t s)
{
  speed = s;
}

void SPI::begin(void)
{
  DDRB &= ~(1 << MISO);
  DDRB |= (1 << MOSI) | (1 << SCK);              //Set MOSI, SCK as Output

  if (speed)
  {
    SPCR = (1 << SPE) | (1 << MSTR);
  }
  else
  {
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0); //Enable SPI, Set as Master, Prescaler: Fosc/16
  }

  SPSR |= (1 << SPI2X);
}

void SPI::stop(void)
{
  SPCR = 0;
  SPSR &= (1 << SPI2X);
}

uint8_t SPI::transfer(uint8_t data)
{
    SPDR = data;                       //Load data into the buffer
    while(!(SPSR & (1 << SPIF)));       //Wait until transmission complete
    return SPDR;
}
