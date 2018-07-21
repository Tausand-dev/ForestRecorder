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
#include "SD/mmc_avr.h"

RTC_DS3231 RTC;

volatile UART serial(BAUDRATE);
volatile UINT Timer;	/* Performance timer (100Hz increment) */
volatile uint8_t error, record;
UINT bw;
FIL *fp;
FATFS *fs;


VS1053 recorder;

ISR(__vector_default){}

ISR(USART_RX_vect)
{
  serial.toBuffer();
}

ISR(TIMER0_COMPA_vect)
{
	Timer;			/* Performance counter for this module */
	disk_timerproc();	/* Drive timer procedure of low level disk I/O module */
}

ISR(INT0_vect)
{
  record = 0;
  serial.println("Interrupted");
}

DWORD get_fattime(void)
{
  RTC_DS3231 RTC;
  DateTime now = RTC.now();

	/* Pack date and time into a DWORD variable */
	return	  ((DWORD)(now.year() - 1980) << 25)
			| ((DWORD)now.month() << 21)
			| ((DWORD)now.day() << 16)
			| ((DWORD)now.hour() << 11)
			| ((DWORD)now.minute() << 5)
			| ((DWORD)now.second() >> 1);
}

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
    else if (func == RESET_COMMAND)
    {

    }
    else
    {
      serial.write(func);
      serial.println("");
      serial.flush();
    }
  }
}

void writeReset(void)
{
  char buffer[10];

  DateTime now = RTC.now();
  ltoa(now.unixtime(), buffer, 10);

  if (f_open(fp, "resets.dat", FA_WRITE | FA_OPEN_APPEND) == FR_OK)
  {
    f_write(fp, buffer, 10, &bw);
    f_write(fp, "\n", 1, &bw);
    f_close(fp);
  }
}

void initSystems(void)
{
  /* Start 100Hz system timer with TC0 */
  OCR0A = (F_CPU / 1024 / 100 - 1);
  TCCR0A = (1 << WGM01);
  TCCR0B = (1 << CS02 | 1 << CS00);
  // TCCR0B = (1 << CS02);
  TIMSK0 = (1 << OCIE0A);

  EICRA |= (1 << ISC01);
  EIMSK |= (1 << EIMSK);

  sei();

  RTC.begin();
  serial.setUART();
  serial.println("Tausand's Forest Recorder");

  if (! recorder.begin())
  {
    serial.println("VS1053 init error");
  }

  fp = (FIL *) malloc(sizeof (FIL));
  fs = (FATFS *) malloc(sizeof(FATFS));

  error = f_mount(fs, "", 1);
  if(error == FR_OK)
  {
    writeReset();
  }
  else
  {
    serial.print("SD Card error: ");
    serial.write(error);
    serial.println("");
  }
}

void makeRecord(const char *name, uint16_t sample_rate, uint16_t seconds)
{
  error = recorder.startRecord(name, sample_rate, 1);
  while(recorder.recordedWordsWaiting() == 0){}

  DateTime next = RTC.now().unixtime() + seconds;

  RTC.clearAlarm(1);
  RTC.setAlarm(ALM1_MATCH_DATE, next.second(), next.minute(), next.hour(), next.day());
  RTC.alarmInterrupt(1, true);

  record = 1;

  while (record)
  {
    error = recorder.saveRecordedData(0);
    if (error != FR_OK)
    {
      serial.write(error);
      serial.println(" saving record");
      break;
    }
  }
  serial.println("Loop done");
  recorder.stopRecord();
  _delay_ms(1000);
}

int main(void)
{
  uint8_t i;
  initSystems();

  _delay_ms(2500);
  for(i = 0; i < 20; i++)
  {
      serialHandler();
      _delay_ms(100);
  }

  makeRecord("Test8.wav", 8000, 60);
  makeRecord("Test16.wav", 16000, 60);
  // error = recorder.startRecord("Test.wav", 8000, 1, EXPAND_SIZE);


  serial.println("Done");

  return 0;
}
