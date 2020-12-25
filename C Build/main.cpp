#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <string.h>

#include "Serial/serial.h"
#include "RTC/rtc.h"
#include "RTC/twi.h"
#include "VS/VS1053.h"
#include "SD/ff.h"
#include "SD/mmc_avr.h"
#include "main.h"

RTC_DS3231 RTC;

volatile UART serial(BAUDRATE);
volatile UINT Timer;	/* Performance timer (100Hz increment) */
volatile uint8_t error, record;
UINT bw;
FIL *fp;
FATFS *fs;

VS1053 recorder;

int main(void)
{
  uint8_t i;
  initSystems();

  /* set clock time */
  _delay_ms(2500);
  for(i = 0; i < 20; i++)
  {
      serialHandler();
      _delay_ms(100);
  }

  // f_open(fp, "v44k1q05.img", FA_READ);
  //
  // for(i = 0; i < 10; i++)
  // {
  //   error = f_readByte(fp);
  //   serial.write(error);
  //   serial.println("");
  // }

  // makeRecordWAV("Test8.wav", 8000, 60);
  // makeRecordWAV("Test16.wav", 16000, 60);

  makeRecordOgg("Test.ogg", 60);
  //
  serial.println("Done");

  return 0;
}

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

void ledFlicker(uint8_t n)
{
  uint8_t i;
  for(i = 0; i < n; i++)
  {
    LED_PORT |= (1 << LED_PIN);
    _delay_ms(LED_DELAY);
    LED_PORT &= ~(1 << LED_PIN);
    _delay_ms(LED_DELAY);
  }
}

void reset(uint8_t error)
{
  serial.print("Error: ");
  serial.write(error);
  serial.println("");

  ledFlicker(error);

  writeReset(error);

  _delay_ms(2500);
  WDTCSR = WDCE;
  WDTCSR |= (1 << WDE) | (1 << WDP1) | (1 << WDP2);
  while(1)
  {
  }
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

void writeReset(uint8_t code)
{
  char buffer[10];
  char code_str[10];

  itoa(code, code_str, 10);

  DateTime now = RTC.now();
  ltoa(now.unixtime(), buffer, 10);

  if (f_open(fp, "resets.dat", FA_WRITE | FA_OPEN_APPEND) == FR_OK)
  {
    f_write(fp, buffer, 10, &bw);
    f_write(fp, " ", 1, &bw);
    f_write(fp, code_str, 2, &bw);
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

  // Alarm interrupt
  // EICRA |= (1 << ISC01);
  // EIMSK |= (1 << EIMSK);

  PORTD |= (1 << PD2); //pull up INT0

  LED_DDR |= (1 << LED_PIN);

  serial.setUART();

  sei();
  serial.println("Tausand's Forest Recorder");

  RTC.begin();

  if (! recorder.begin())
  {
    reset(VS1053_ERROR);
  }

  fp = (FIL *) malloc(sizeof (FIL));
  fs = (FATFS *) malloc(sizeof(FATFS));

  error = f_mount(fs, "", 1);
  if(error != FR_OK)
  {
    reset(SD_ERROR);
  }

  ledFlicker(1);
}

void activateINT0(void)
{
  EICRA |= (1 << ISC01);
  EIMSK |= (1 << EIMSK);
}

void deactivateINT0(void)
{
  EICRA &= ~(1 << ISC01);
  EIMSK &= ~(1 << EIMSK);
}

void makeRecordWAV(const char *name, uint16_t sample_rate, uint16_t seconds)
{
  error = recorder.startRecordWAV(name, sample_rate, 1);
  while(recorder.recordedWordsWaiting() == 0){}

  DateTime next = RTC.now().unixtime() + seconds;

  RTC.clearAlarm(1);
  RTC.setAlarm(ALM1_MATCH_DATE, next.second(), next.minute(), next.hour(), next.day());
  RTC.alarmInterrupt(1, true);

  activateINT0();

  record = 1;

  while (record)
  {
    error = recorder.saveRecordedData(0);
    if (error != FR_OK)
    {
      reset(WRITE_ERROR);
      break;
    }
  }

  serial.println("Loop done");
  recorder.stopRecordWAV();
  _delay_ms(1000);
}

void makeRecordOgg(const char *name, uint16_t seconds)
{
  error = recorder.startRecordOgg(name, 0, 1);
  if(error == 4)
  {
    reset(READ_PLUGIN_ERROR);
  }

  while(recorder.recordedWordsWaiting() == 0){}

  DateTime next = RTC.now().unixtime() + seconds;

  RTC.clearAlarm(1);
  RTC.setAlarm(ALM1_MATCH_DATE, next.second(), next.minute(), next.hour(), next.day());
  RTC.alarmInterrupt(1, true);

  activateINT0();
  record = 1;

  while (record > 0)
  {
    error = recorder.saveRecordedData(0);
    if (error != FR_OK)
    {
      serial.write(error);
      serial.println(" saving record");
      reset(WRITE_ERROR);
      break;
    }
  }
  deactivateINT0();
  serial.println("Loop done");
  // recorder.stopRecord();
  _delay_ms(1000);
}
