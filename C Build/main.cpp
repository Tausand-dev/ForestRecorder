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
// #include "SD/diskio.h"
// #include "SD/mmc_avr.h"

RTC_DS3231 RTC;
UART serial(BAUDRATE);

FATFS FatFs;	// FatFs work area
FIL *fp; // fpe object
uint8_t BUFFER[128];

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

ISR(TIMER0_COMPA_vect)
{
	Timer1++;			/* Performance counter for this module */
	mmc_disk_timerproc();	/* Drive timer procedure of low level disk I/O module */
}

// static void ioinit (void)
// {
// 	// MCUCR = _BV(JTD); MCUCR = _BV(JTD);	/* Disable JTAG */
//
// 	/* Start 100Hz system timer with TC0 */
// 	OCR0A = F_CPU / 1024 / 100 - 1;
// 	TCCR0A = _BV(WGM01);
// 	TCCR0B = 0b101;
// 	TIMSK0 = _BV(OCIE0A);
// }

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

int main(void)
{
  ioinit();
  RTC.begin();
  serial.setUART();
  serial.println("Connection");

  sei();

  mount();

  // VS1053 recorder;
  // uint8_t error = recorder.begin();



  // if (! error)
  // {
  //   serial.print("VS1053 init error: ");
  //   serial.write(error);
  //   serial.println("");
  // }



  while(1)
  {
    serialHandler();
  }

  return 0;
}

void mount(void)
{
    uint8_t temp;

  	temp = f_mount(&FatFs, "", 1);		// Give a work area to the FatFs module
    if (temp == FR_OK)
    {
      serial.println("F_MOUNT: OK");

      fp = (FIL *)malloc(sizeof (FIL));
      temp = f_open(fp, "0:file.txt", FA_WRITE | FA_CREATE_ALWAYS);

      if (temp == FR_OK)
    	{
        UINT bw;
    		const char *text = "Hello World! SDCard support up and running!\r\n";
    		f_write(fp, text, strlen(text), &bw);	// Write data to the file
    		f_close(fp);// Close the file
    		serial.println("File written");
    	}
    	else
    	{
    		serial.print("file error: ");
    		serial.write(temp);
    		serial.println("");
      }
    }
    else
    {
      serial.print("F_MOUNT error: ");
      serial.write(temp);
      serial.println("");
    }
}
