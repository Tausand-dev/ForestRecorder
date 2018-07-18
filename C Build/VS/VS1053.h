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
#ifndef VS1053_H
#define VS1053_H

#include <stdint.h>
#include "../SPI/SPI.h"

/*
#include <BlockDriver.h>
#include <FreeStack.h>
#include <MinimumSerial.h>
#include <SdFat.h>
#include <SdFatConfig.h>
#include <SysCall.h>
*/

#define RESET PB1
#define RESET_PIN PINB
#define RESET_PORT PORTB
#define RESET_DDR DDRB

#define CS PB2
#define CS_PIN PINB
#define CS_PORT PORTB
#define CS_DDR DDRB

#define DREQ PD5
#define DREQ_PIN PIND
#define DREQ_PORT PORTD
#define DREQ_DDR DDRD

#define DCS PB0
#define DCS_PIN PINB
#define DCS_PORT PORTB
#define DCS_DDR DDRB

#define VS1053_FILEPLAYER_TIMER0_INT 255 // allows useInterrupt to accept pins 0 to 254
#define VS1053_FILEPLAYER_PIN_INT 5

#define VS1053_SCI_READ 0x03
#define VS1053_SCI_WRITE 0x02

#define VS1053_REG_MODE  0x00
#define VS1053_REG_STATUS 0x01
#define VS1053_REG_BASS 0x02
#define VS1053_REG_CLOCKF 0x03
#define VS1053_REG_DECODETIME 0x04
#define VS1053_REG_AUDATA 0x05
#define VS1053_REG_WRAM 0x06
#define VS1053_REG_WRAMADDR 0x07
#define VS1053_REG_HDAT0 0x08
#define VS1053_REG_HDAT1 0x09
#define VS1053_REG_VOLUME 0x0B

#define VS1053_GPIO_DDR 0xC017
#define VS1053_GPIO_IDATA 0xC018
#define VS1053_GPIO_ODATA 0xC019

#define VS1053_INT_ENABLE  0xC01A

#define VS1053_MODE_SM_DIFF 0x0001
#define VS1053_MODE_SM_LAYER12 0x0002
#define VS1053_MODE_SM_RESET 0x0004
#define VS1053_MODE_SM_CANCEL 0x0008
#define VS1053_MODE_SM_EARSPKLO 0x0010
#define VS1053_MODE_SM_TESTS 0x0020
#define VS1053_MODE_SM_STREAM 0x0040
#define VS1053_MODE_SM_SDINEW 0x0800
#define VS1053_MODE_SM_ADPCM 0x1000
#define VS1053_MODE_SM_LINE1 0x4000
#define VS1053_MODE_SM_CLKRANGE 0x8000

#define VS1053_SCI_AIADDR 0x0A
#define VS1053_SCI_AICTRL0 0x0C
#define VS1053_SCI_AICTRL1 0x0D
#define VS1053_SCI_AICTRL2 0x0E
#define VS1053_SCI_AICTRL3 0x0F

#define VS1053_DATABUFFERLEN 32

// const unsigned short plugin[] = { /* Compressed plugin */
//   0x0007,0x0001, /*copy 1*/
//   0x8050,
//   0x0006,0x0042, /*copy 66*/
//   0x0000,0x1790,0xf400,0x5400,0x0000,0x0a10,0xf400,0x5600,
//   0xb080,0x0024,0x0007,0x9257,0x3f00,0x0024,0x0030,0x0297,
//   0x3f00,0x0024,0x0000,0x004d,0x0014,0x958f,0x0000,0x1b4e,
//   0x280f,0xe100,0x0006,0x2016,0x2a00,0x17ce,0x3e12,0xb817,
//   0x3e14,0xf812,0x3e01,0xb811,0x0007,0x9717,0x0020,0xffd2,
//   0x0030,0x11d1,0x3111,0x8024,0x3704,0xc024,0x3b81,0x8024,
//   0x3101,0x8024,0x3b81,0x8024,0x3f04,0xc024,0x2808,0x4800,
//   0x36f1,0x9811,0x2814,0x9c91,0x0000,0x004d,0x2814,0x9940,
//   0x003f,0x0013,
//   0x000a,0x0001,
//   0x0050,
// };



class VS1053
{
  public:
    uint8_t begin(void);
    void reset(void);
    void softReset(void);
    bool readyForData(void);
    uint16_t sciRead(uint8_t addr);
    void sciWrite(uint8_t addr, uint16_t data);
    void spiwrite(uint8_t d);
    void spiwrite(uint8_t *c, uint16_t num);
    uint8_t spiread(void);

    // uint16_t loadPlugin(char *fn);
    void loadPlugin(void);

    void prepareRecord(void);
    void startRecord(bool mic);

    bool prepareRecordOgg(char *plugin);
    void startRecordOgg(bool mic);
    void stopRecordOgg(void);
    uint16_t recordedWordsWaiting(void);
    uint16_t recordedReadWord(void);

  private:
    SPI spi;
};

#endif
