/***************************************************
  This is a library for the Adafruit VS1053 Codec Breakout

  Designed specifically to work with the Adafruit VS1053 Codec Breakout
  ----> https://www.adafruit.com/products/1381

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include <avr/io.h>
#include <util/delay.h>
#include "VS1053.h"
#include "../SPI/SPI.h"

// void VS1053::loadPlugin(void)
// {
  // unsigned int i = 0;
  // while (i < (sizeof(plugin) / sizeof(plugin[0])))
  // {
  //   unsigned short addr, n, val;
  //   addr = plugin[i++];
  //   n = plugin[i++];
  //   if (n & 0x8000U)
  //   { /* RLE run, replicate n samples */
  //     n &= 0x7FFF;
  //     val = plugin[i++];
  //     while (n--)
  //     {
  //       sciWrite(addr, val);
  //     }
  //   }
  //   else
  //   {
  //     while (n--)
  //     {
  //       val = plugin[i++];
  //       sciWrite(addr, val);
  //     }
  //   }
  // }
// }

bool VS1053::readyForData(void)
{
  return digitalRead(&DREQ_PIN, DREQ);
}

void VS1053::softReset(void)
{
  sciWrite(VS1053_REG_MODE, VS1053_MODE_SM_SDINEW | VS1053_MODE_SM_RESET);
  while(! readyForData());
}

void VS1053::reset()
{
  // TODO: http://www.vlsi.fi/player_vs1011_1002_1003/modularplayer/vs10xx_8c.html#a3
  // hardware reset
  digitalWrite(&RESET_PORT, RESET, 0);
  _delay_ms(100);
  digitalWrite(&RESET_PORT, RESET, 1);

  digitalWrite(&CS_PORT, CS, 1);
  digitalWrite(&DCS_PORT, DCS, 1);
  _delay_ms(100);
  softReset();
  _delay_ms(100);

  sciWrite(VS1053_REG_CLOCKF, 0x6000);
}

uint8_t VS1053::begin(void)
{
  RESET_DDR |= (1 << RESET); // reset as output
  CS_DDR |= (1 << CS); // cs as output
  DCS_DDR |= (1 << DCS);
  DREQ_DDR &= ~(1 << DREQ);

  digitalWrite(&RESET_PORT, RESET, 0);
  digitalWrite(&CS_PORT, CS, 1);
  digitalWrite(&DCS_PORT, DCS, 1);

  spi.begin();
  reset();

  uint8_t val = (sciRead(VS1053_REG_STATUS) >> 4) & 0x0F;
  return (val == 4);
}

uint16_t VS1053::recordedWordsWaiting(void)
{
  return sciRead(VS1053_REG_HDAT1);
}

uint16_t VS1053::recordedReadWord(void)
{
  return sciRead(VS1053_REG_HDAT0);
}

void VS1053::loadPlugin(void)
{
  sciWrite(VS1053_REG_WRAMADDR, 0x8010);
  sciWrite(VS1053_REG_WRAM, 0x3e12);
  sciWrite(VS1053_REG_WRAM, 0xb817);
  sciWrite(VS1053_REG_WRAM, 0x3e14);
  sciWrite(VS1053_REG_WRAM, 0xf812);
  sciWrite(VS1053_REG_WRAM, 0x3e01);
  sciWrite(VS1053_REG_WRAM, 0xb811);
  sciWrite(VS1053_REG_WRAM, 0x0007);
  sciWrite(VS1053_REG_WRAM, 0x9717);
  sciWrite(VS1053_REG_WRAM, 0x0020);
  sciWrite(VS1053_REG_WRAM, 0xffd2);
  sciWrite(VS1053_REG_WRAM, 0x0030);
  sciWrite(VS1053_REG_WRAM, 0x11d1);
  sciWrite(VS1053_REG_WRAM, 0x3111);
  sciWrite(VS1053_REG_WRAM, 0x8024);
  sciWrite(VS1053_REG_WRAM, 0x3704);
  sciWrite(VS1053_REG_WRAM, 0xc024);
  sciWrite(VS1053_REG_WRAM, 0x3b81);
  sciWrite(VS1053_REG_WRAM, 0x8024);
  sciWrite(VS1053_REG_WRAM, 0x3101);
  sciWrite(VS1053_REG_WRAM, 0x8024);
  sciWrite(VS1053_REG_WRAM, 0x3b81);
  sciWrite(VS1053_REG_WRAM, 0x8024);
  sciWrite(VS1053_REG_WRAM, 0x3f04);
  sciWrite(VS1053_REG_WRAM, 0xc024);
  sciWrite(VS1053_REG_WRAM, 0x2808);
  sciWrite(VS1053_REG_WRAM, 0x4800);
  sciWrite(VS1053_REG_WRAM, 0x36f1);
  sciWrite(VS1053_REG_WRAM, 0x9811);
  sciWrite(VS1053_REG_WRAMADDR, 0x8028);
  sciWrite(VS1053_REG_WRAM, 0x2a00);
  sciWrite(VS1053_REG_WRAM, 0x040e);
}

void VS1053::startRecord(bool mic)
{
  softReset();
  while(! readyForData());

  sciWrite(VS1053_SCI_AICTRL0, 16000U);
  sciWrite(VS1053_SCI_AICTRL1, 0);
  sciWrite(VS1053_SCI_AICTRL2, 4096U);
  sciWrite(VS1053_SCI_AICTRL3, 0);
  // sciWrite(VS1053_SCI_AICTRL3, (1 << 2));
  uint16_t config = 0;

  config = VS1053_MODE_SM_RESET | VS1053_MODE_SM_ADPCM | VS1053_MODE_SM_SDINEW;
  if (! mic)
  {
    config |= VS1053_MODE_SM_LINE1;
  }

  sciWrite(VS1053_REG_MODE, config);
  while (! readyForData() );

  loadPlugin();
}

uint16_t VS1053::sciRead(uint8_t addr)
{
  uint16_t data;

  spi.begin();
  digitalWrite(&CS_PORT, CS, 0);
  spiwrite(VS1053_SCI_READ);
  spiwrite(addr);

  _delay_us(10);

  data = spiread();
  data <<= 8;
  data |= spiread();

  digitalWrite(&CS_PORT, CS, 1);
  return data;
}

void VS1053::sciWrite(uint8_t addr, uint16_t data)
{
  spi.begin();
  digitalWrite(&CS_PORT, CS, 0);
  spiwrite(VS1053_SCI_WRITE);
  spiwrite(addr);
  spiwrite(data >> 8);
  spiwrite(data & 0xFF);
  digitalWrite(&CS_PORT, CS, 1);
  // SPI.endTransaction();
}

uint8_t VS1053::spiread(void)
{
  int8_t x;
  x = 0;
  x = spi.transfer(0x00);
  return x;
}

void VS1053::spiwrite(uint8_t c)
{
  uint8_t x __attribute__ ((aligned (32))) = c;
  spiwrite(&x, 1);
}

void VS1053::spiwrite(uint8_t *c, uint16_t num)
{
  while (num--)
  {
    spi.transfer(c[0]);
    c++;
  }
}
