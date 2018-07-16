#ifndef _SPI_H_INCLUDED
#define _SPI_H_INCLUDED

class SPI
{
  public:
    SPI(void);
    void begin(void);
    uint8_t transfer(uint8_t);
};

#endif
