#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <string.h>

#include "Serial/serial.h"
#include "RTC/rtc.h"
#include "RTC/twi.h"
#include "VS/VS1053.h"
#include "test.h"

RTC_DS3231 RTC;
UART serial(BAUDRATE);

ISR(__vector_default){}

ISR(USART_RX_vect)
{
  serial.toBuffer();
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
  }
}

int main(void)
{
  serial.setUART();
  serial.println("Connection");

  sei();

  RTC.begin();

  VS1053 vs;
  uint8_t ans = vs.begin();
  serial.print("VS1053: ");
  serial.write(ans);
  serial.println("");

  while(1)
  {
    serialHandler();
    // _delay_ms(10);
  }

  return 0;
}
