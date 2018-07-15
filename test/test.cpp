#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <string.h>

#include "serial.h"
#include "rtc.h"
#include "twi.h"

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

  RTC.begin();

  DateTime time(2018, 8, 13);

  RTC.adjust(time);

  while(1)
  {
    DateTime now = RTC.now();
    serial.write(now.unixtime());
    serial.println("");
    _delay_ms(1000);
  }

  return 0;
}
