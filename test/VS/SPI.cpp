#include <avr/io.h>
#include <stdint.h>
#include "SPI.h"

SPI::SPI(void)
{

}

void SPI::begin(void)
{
    DDRB = (1 << PB3) | (1 << PB5);              //Set MOSI, SCK as Output
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0); //Enable SPI, Set as Master, Prescaler: Fosc/16
}

uint8_t SPI::transfer(uint8_t data)
{
    SPDR = data;                       //Load data into the buffer
    while(!(SPSR & (1 << SPIF)));       //Wait until transmission complete
    return SPDR;
}
