#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "serial.h"
#include "RTC.h"
#include <string.h>

#define BAUDRATE 57600

RTC_DS3231 RTC;
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

  if (! RTC.begin())
  {
      serial.println("Couldn't find RTC");
  }

  DateTime time(2018, 8, 13);

  RTC.adjust(time);

  DateTime now = RTC.now();

  serial.write(time.unixtime());
  serial.write(now.unixtime());

  return 0;
}
