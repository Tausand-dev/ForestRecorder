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
// #include "SD/ff.h"

RTC_DS3231 RTC;
VS1053 recorder;
UART serial(BAUDRATE);

// FATFS FatFs;	// FatFs work area
// uint8_t BUFFER[128];

ISR(__vector_default){}

ISR(USART_RX_vect)
{
  serial.toBuffer();
}

// DWORD get_fattime (void)
// {
//   DateTime now = RTC.now();
//
// 	/* Pack date and time into a DWORD variable */
// 	return	  ((DWORD)(now.year() - 1980) << 25)
// 			| ((DWORD)now.month() << 21)
// 			| ((DWORD)now.day() << 16)
// 			| ((DWORD)now.hour() << 11)
// 			| ((DWORD)now.minute() << 5)
// 			| ((DWORD)now.second() >> 1);
// }

// void mount(void);

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
  sei();
  RTC.begin();
  serial.setUART();

  serial.println("Connection");

  // if (f_mount(0, &FatFs) != FR_OK)
  // {
  //   serial.println("SD error");
  // }

  if (! recorder.begin())
  {
    serial.println("VS1053 init error");
  }

}

int main(void)
{
  initSystems();

  serial.write(recorder.sciRead(VS1053_REG_CLOCKF));
  serial.println("");

  _delay_ms(1000);
  recorder.startRecord(1);

  while(1)
  {
    serial.write(recorder.recordedWordsWaiting());
    serial.println("");
    _delay_ms(1000);
    // serialHandler();
  }

  return 0;
}

// void mount(void)
// {
//   UINT bw;
//
//   FIL *fp;
//   fp = (FIL *) malloc(sizeof (FIL));
//   uint8_t temp;
//
//   temp = f_open(fp, "file.txt", FA_WRITE | FA_CREATE_ALWAYS);
//
//   if (temp == FR_OK)
//   {	// Create a file
//     const char *text = "Hello World! SDCard support up and running!\r\n";
//     f_write(fp, text, strlen(text), &bw);	// Write data to the file
//     f_close(fp);// Close the file
//     serial.println("File written");
//   }
//   else
//   {
//     serial.print("file error: ");
//     serial.write(temp);
//     serial.println("");
//   }
// }
