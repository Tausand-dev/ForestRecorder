#ifndef _SPI_H_INCLUDED
#define _SPI_H_INCLUDED

#define MOSI PB3
#define MISO PB4
#define SCK PB5

class SPI
{
  public:
    SPI(void);
    void begin(void);
    uint8_t transfer(uint8_t);
};

uint8_t digitalRead(volatile uint8_t *pin, uint8_t n);
void digitalWrite(volatile uint8_t *port, uint8_t n, uint8_t val);


#endif
