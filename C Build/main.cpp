#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <string.h>

#include "Serial/serial.h"
#include "RTC/rtc.h"
#include "RTC/twi.h"
#include "VS/VS1053.h"
#include "main.h"
#include "SD/ff.h"

RTC_DS3231 RTC;
VS1053 recorder;
UART serial(BAUDRATE);

FATFS FatFs;

ISR(__vector_default){}

ISR(USART_RX_vect)
{
  serial.toBuffer();
}

DWORD get_fattime (void)
{
  DateTime now = RTC.now();

	/* Pack date and time into a DWORD variable */
	return	  ((DWORD)(now.year() - 1980) << 25)
			| ((DWORD)now.month() << 21)
			| ((DWORD)now.day() << 16)
			| ((DWORD)now.hour() << 11)
			| ((DWORD)now.minute() << 5)
			| ((DWORD)now.second() >> 1);
}

void mount(void);

void sendTime(void)
{
  DateTime now = RTC.now();
  serial.write(now.unixtime());
  serial.println("");
}

void serialHandler(void)
{
  uint8_t func;
  uint8_t m1, m2, m3, m4;
  uint32_t incoming;

  if (serial.available())
  {
    func = serial.read();
    if(func == SET_TIME_COMMAND)
    {
        _delay_ms(50);
        if(serial.available() >= 4)
        {
          m1 = serial.read();
          m2 = serial.read();
          m3 = serial.read();
          m4 = serial.read();
          incoming = get32From8Bit(m1, m2, m3, m4);
          DateTime time(incoming);
          RTC.adjust(time);
          sendTime();
        }
        else
        {
          serial.write(0);
          serial.println("");
        }
    }
    else if (func == GET_TIME_COMMAND)
    {
      sendTime();
    }
  }
}

void initSystems(void)
{
  _delay_ms(1000);
  sei();
  RTC.begin();
  serial.setUART();

  serial.println("Connection");

  if (f_mount(0, &FatFs) != FR_OK)
  {
    serial.println("SD error");
  }

  if (! recorder.begin())
  {
    serial.println("VS1053 init error");
  }

}

int main(void)
{
  initSystems();

  // _delay_ms(100);
  // recorder.startRecord("Test.wav", 1);
  // _delay_ms(100);

  // uint8_t i;
  //
  // for(i = 0; i < 200; i++)
  // {
  //   if(! recorder.saveRecordedData(0))
  //   {
  //     serial.println("Error saving data");
  //   }
  // }

  // recorder.saveRecordedData(1);

  mount();

  serial.println("Done");

  return 0;
}

void mount(void)
{
  UINT bw;
  FIL *fp;
  fp = (FIL *) malloc(sizeof (FIL));
  if (f_open(fp, "text.txt", FA_WRITE) == FR_OK)
  {
    f_write(fp, "Hello World\n", 12, &bw);	// Write data to the file
    f_close(fp);// Close the file
    // return 1;
  }
}
