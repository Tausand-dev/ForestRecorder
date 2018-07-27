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
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>
#include "VS1053.h"
#include "../SPI/SPI.h"

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
  spi.setSpeed(0);
  digitalWrite(&RESET_PORT, RESET, 0);
  _delay_ms(100);
  digitalWrite(&RESET_PORT, RESET, 1);

  digitalWrite(&CS_PORT, CS, 1);
  digitalWrite(&DCS_PORT, DCS, 1);
  _delay_ms(100);
  softReset();
  _delay_ms(100);
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

  reset();
  fp = (FIL *) malloc(sizeof (FIL));

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

uint8_t VS1053::readPlugin(const char *plugname)
{
  uint8_t error;
  error = f_open(fp, plugname, FA_READ);

  if (error != FR_OK)
  {
    // Serial.println("Couldn't open the plugin file");
    // Serial.println(plugname);
    return error;
  }

  else if((f_readByte(fp) != 'P') ||
        (f_readByte(fp) != '&') ||
        (f_readByte(fp) != 'H'))
    return 0xFF;

  uint16_t type;

 // Serial.print("Patch size: "); Serial.println(patchsize);
  while ((type = f_readByte(fp)) >= 0)
  {
    uint16_t offsets[] = {0x8000UL, 0x0, 0x4000UL};
    uint16_t addr, len;

    if (type >= 4)
    {
      f_close(fp);
      return 0xFF;
    }

    len = f_readByte(fp);    len <<= 8;
    len |= f_readByte(fp) & ~1;
    addr = f_readByte(fp);    addr <<= 8;
    addr |= f_readByte(fp);

    if (type == 3)
    {
      // execute rec!
      f_close(fp);
      return addr;
    }

    // set address
    sciWrite(VS1053_REG_WRAMADDR, addr + offsets[type]);
    // write data
    while(len -= 2)
    {
      uint16_t data;
      data = f_readByte(fp);    data <<= 8;
      data |= f_readByte(fp);
      sciWrite(VS1053_REG_WRAM, data);
    }
  }

  f_close(fp);
  return 0xFF;
}

bool VS1053::prepareRecordOgg(const char *plugname)
{
  sciWrite(VS1053_REG_CLOCKF, 0xC000);  // set max clock
  _delay_ms(1);    while (! readyForData() );

  sciWrite(VS1053_REG_BASS, 0);  // clear Bass

  softReset();
  _delay_ms(1);    while (! readyForData() );

  sciWrite(VS1053_SCI_AIADDR, 0);
  // disable all interrupts except SCI
  sciWrite(VS1053_REG_WRAMADDR, VS1053_INT_ENABLE);
  sciWrite(VS1053_REG_WRAM, 0x02);

  int pluginStartAddr = readPlugin(plugname);
  if (pluginStartAddr == 0xFF) return false;
  // Serial.print("Plugin at $"); Serial.println(pluginStartAddr, HEX);
  if (pluginStartAddr != 0x34) return false;

  return true;
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

// uint8_t VS1053::startRecord(const char *name, uint16_t sample_rate, bool mic, uint32_t expand_size)
uint8_t VS1053::startRecordWAV(const char *name, uint16_t sample_rate, bool mic)
{
  softReset();
  while(! readyForData());

  sciWrite(VS1053_REG_CLOCKF, 0x6000);
  spi.setSpeed(1);
  _delay_ms(100);
  while(! readyForData());

  sciWrite(VS1053_SCI_AICTRL0, sample_rate);
  sciWrite(VS1053_SCI_AICTRL1, 1);
  sciWrite(VS1053_SCI_AICTRL2, 0);
  // sciWrite(VS1053_SCI_AICTRL3, 0);
  sciWrite(VS1053_SCI_AICTRL3, 2 | (1 << 2));
  uint16_t config = 0;

  // config = VS1053_MODE_SM_RESET;
  // config = VS1053_MODE_SM_RESET | VS1053_MODE_SM_ADPCM | VS1053_MODE_SM_SDINEW;
  if (mic)
  {
    config = VS1053_MODE_SM_ADPCM;
  }
  else
  {
    config = VS1053_MODE_SM_ADPCM | VS1053_MODE_SM_LINE1;
  }

  sciWrite(VS1053_REG_MODE, config);
  while (! readyForData() );
  loadPlugin();
  while (! readyForData() );

  /* write "RIFF" */
  buffer[0] = 'R';
  buffer[1] = 'I';
  buffer[2] = 'F';
  buffer[3] = 'F';

  /* write "WAVE" */
  buffer[8] = 'W';
  buffer[9] = 'A';
  buffer[10] = 'V';
  buffer[11] = 'E';

  /* write "fmt " */
  buffer[12] = 'f';
  buffer[13] = 'm';
  buffer[14] = 't';
  buffer[15] = ' ';

  /*subchunk1D*/
  buffer[16] = 0x10;
  buffer[17] = 0;
  buffer[18] = 0;
  buffer[19] = 0;

  /*audio format*/
  buffer[20] = 0x01;
  buffer[21] = 0;

  /*number channels*/
  buffer[22] = 0x01;
  buffer[23] = 0;

  /*sample rate*/
  buffer[24] = sample_rate;
  buffer[25] = sample_rate >> 8;
  buffer[26] = 0;
  buffer[27] = 0;

  /*Byte rate*/
  buffer[28] = 2*sample_rate;
  buffer[29] = (2*sample_rate >> 8);
  buffer[30] = 0;
  buffer[31] = 0;

  /*Block align*/
  buffer[32] = 0x02;
  buffer[33] = 0;

  /*Bits sample*/
  buffer[34] = 0x10;
  buffer[35] = 0;

  /*Byte rate*/
  buffer[36] = 'd';
  buffer[37] = 'a';
  buffer[38] = 't';
  buffer[39] = 'a';

  uint8_t error;
  error = f_open(fp, name, FA_WRITE | FA_CREATE_ALWAYS);
  // error = f_expand(fp, expand_size, 1);
  // if (error != FR_OK)
  // {
  //   return error;
  // }
  //
  // f_lseek(fp, 0);
  f_write(fp, buffer, 44, &bw);
  f_sync(fp);

  return error;
}

uint8_t VS1053::startRecordOgg(const char *name, uint16_t sample_rate, bool mic)
{
  /* Set VS1053 mode bits as instructed in the VS1053b Ogg Vorbis Encoder
     manual. Note: for microphone input, leave SMF_LINE1 unset! */

  uint8_t error;
  error = f_open(fp, name, FA_WRITE | FA_CREATE_ALWAYS);
  if (error != FR_OK)
  {
    return error;
  }

  error = prepareRecordOgg("v44k1q05.img");
  if(! error)
  {
    return error;
  }

  sciWrite(VS1053_REG_CLOCKF, 0x6000);
  spi.setSpeed(1);
  _delay_ms(100);
  while(! readyForData());

  if (mic)
  {
    sciWrite(VS1053_REG_MODE, VS1053_MODE_SM_ADPCM | VS1053_MODE_SM_SDINEW);
  }
  else
  {
    sciWrite(VS1053_REG_MODE, VS1053_MODE_SM_LINE1 |
	     VS1053_MODE_SM_ADPCM | VS1053_MODE_SM_SDINEW);
  }
  sciWrite(VS1053_SCI_AICTRL0, 1024);
  /* Rec level: 1024 = 1. If 0, use AGC */
  sciWrite(VS1053_SCI_AICTRL1, 1024);
  /* Maximum AGC level: 1024 = 1. Only used if SCI_AICTRL1 is set to 0. */
  sciWrite(VS1053_SCI_AICTRL2, 0);
  /* Miscellaneous bits that also must be set before recording. */
  sciWrite(VS1053_SCI_AICTRL3, 0);

  sciWrite(VS1053_SCI_AIADDR, 0x34);
  _delay_ms(1);    while (! readyForData() );

  return error;
}

void VS1053::stopRecord(void)
{
  sciWrite(VS1053_SCI_AICTRL3, 1);
  while (! readyForData() );
  saveRecordedData(1);
}

void VS1053::stopRecordWAV(void)
{
  stopRecord();
  endHeaderWAV();
}

uint8_t VS1053::saveRecordedData(uint8_t wrap)
{
  uint8_t x, error;
  uint16_t addr, t;
  uint16_t waiting = recordedWordsWaiting(); // read how many words are waiting
  while (waiting >= VS1053_MWORDS) // try to process 256 words (512 bytes) at a time, for best speed
  {
    for (x = 0; x < VS1053_MBYTES / VS1053_RECBUFFSIZE; x++) // for example 128 bytes x 4 loops = 512 bytes
    {
      for (addr = 0; addr < VS1053_RECBUFFSIZE; addr += 2)
      {
        t = recordedReadWord();
        buffer[addr] = t >> 8;
        buffer[addr + 1] = t;
      }
      f_write(fp, buffer, VS1053_RECBUFFSIZE, &bw);
    }
    error = f_sync(fp);
    if (error != FR_OK)
    {
      return error;
    }
    waiting -= VS1053_MWORDS;
  }

  if (wrap)
  {
    waiting = recordedWordsWaiting();
    for (addr = 0; addr < waiting - 1; addr++)
    {
      t = recordedReadWord();
      buffer[addr] = t >> 8;
      buffer[addr + 1] = t;
      if (addr > VS1053_RECBUFFSIZE)
      {
        addr = 0;
        f_write(fp, buffer, VS1053_RECBUFFSIZE, &bw);
        error = f_sync(fp);
      }

      if (error != FR_OK)
      {
        return error;
      }
    }
    if (addr != 0)
    {
      f_write(fp, buffer, addr, &bw);
      error = f_sync(fp);
      if (error != FR_OK)
      {
        return error;
      }
    }

    if (! (sciRead(VS1053_SCI_AICTRL3) & (1 << 2)))
    {
      buffer[0] = recordedReadWord() & 0xFF;
      f_write(fp, buffer, 1, &bw);
      error = f_sync(fp);
      if (error != FR_OK)
      {
        return error;
      }
    }
  }
  return FR_OK;
}

uint8_t VS1053::endHeaderWAV(void)
{
  uint8_t error;
  FSIZE_t size = f_size(fp);
  // size = 1798768;
  FSIZE_t chunckSize = size - 8;
  FSIZE_t chunckSize3 = size - 36;

  error = f_lseek(fp, 4);
  if (error == FR_OK)
  {
    buffer[0] = chunckSize;
    buffer[1] = chunckSize >> 8;
    buffer[2] = chunckSize >> 16;
    buffer[3] = chunckSize >> 24;
    error = f_write(fp, buffer, 4, &bw);
    f_sync(fp);
  }

  error = f_lseek(fp, 40);
  if (error == FR_OK)
  {
    buffer[0] = chunckSize3;
    buffer[1] = chunckSize3 >> 8;
    buffer[2] = chunckSize3 >> 16;
    buffer[3] = chunckSize3 >> 24;
    error = f_write(fp, buffer, 4, &bw);
    f_sync(fp);
  }

  // f_truncate(fp);
  f_close(fp);

  return error;
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
  spi.stop();
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
  spi.stop();
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
