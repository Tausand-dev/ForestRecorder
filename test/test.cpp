#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <string.h>

#include "serial.h"
#include "rtc.h"
#include "twi.h"
#include "test.h"

RTC_DS3231 RTC;
UART serial(BAUDRATE);

ISR(__vector_default){}

ISR(USART_RX_vect)
{
  serial.toBuffer();
}

void serialHandler(void)
{
  uint8_t func;
  uint32_t incoming;

  if (serial.available())
  {
    func = serial.read();
    if((func == SET_TIME) & (serial.available() >= 4))
    {
      incoming = get32From8Bit(serial.read(), serial.read(), serial.read(), serial.read());
      DateTime time(incoming);
      RTC.adjust(time);
    }
    else if (func == GET_TIME)
    {
      DateTime now = RTC.now();
      serial.write(now.unixtime());
      serial.println("");
    }
  }
}

int main(void)
{
  serial.setUART();
  serial.println("Connection");

  sei();

  RTC.begin();

  while(1)
  {
    serialHandler();
  }

  return 0;
}
