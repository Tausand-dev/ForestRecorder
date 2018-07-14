#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "serial.h"
#include "RTC.h"

#define BAUDRATE 57600

RTC_DS3231 RTC;
UART serial(BAUDRATE);

ISR(USART_RX_vect)
{
  serial.toBuffer();
}

//ISR(__vector_default){}

int main(void)
{
  serial.setUART();
  serial.println("Hello World!");

  sei();

  
  if (! RTC.begin())
  {
      serial.println("Couldn't find RTC");
  }

  DateTime now = RTC.now();
  

  return 0;
}
