#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <string.h>

#include "serial.h"
#include "rtc.h"
#include "twi.h"

#define BAUDRATE 57600

// RTC_DS3231 RTC;
UART serial(BAUDRATE);

ISR(__vector_default){}

ISR(USART_RX_vect)
{
  serial.toBuffer();
}

int main(void)
{
  serial.setUART();
  serial.println("Hello World!");

  sei();

  twi_init_master();
  rtc_init();

  struct tm* t = NULL;
  t->sec = 1;      // 0 to 59
  t->min = 2;      // 0 to 59
  t->hour = 3;     // 0 to 23
  t->mday = 4;     // 1 to 31
  t->mon = 5;      // 1 to 12
  t->year = 6;     // year-99

  rtc_set_time(t);

  t = rtc_get_time();

  uint8_t val = t->sec;
  serial.write(val);

  val = t->min;
  serial.write(val);

  val = t->hour;
  serial.write(val);

  val = t->mday;
  serial.write(val);

  val = t->mon;
  serial.write(val);

  val = t->year;
  serial.write(val);

  // if (! RTC.begin())
  // {
  //     serial.println("Couldn't find RTC");
  // }

  // DateTime time(2018, 8, 13);
  //
  // RTC.adjust(time);
  //
  // DateTime now = RTC.now();
  //
  // serial.write(time.unixtime());
  // serial.write(now.unixtime());

  return 0;
}
