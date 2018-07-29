#include <stdint.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "SD/ff.h"
#include "SD/mmc_avr.h"

#define LED_PORT PORTC
#define LED_DDR DDRC
#define LED_PIN PC3
#define LED_DELAY 150

#define MOSI PB3
#define MISO PB4
#define SCK PB5

UINT bw;
FIL *fp;
FATFS *fs;

volatile UINT Timer;	/* Performance timer (100Hz increment) */

DWORD get_fattime(void)
{
	/* Pack date and time into a DWORD variable */
	return	  ((DWORD)(2017 - 1980) << 25)
			| ((DWORD)12 << 21)
			| ((DWORD)12 << 16)
			| ((DWORD)6 << 11)
			| ((DWORD)6 << 5)
			| ((DWORD)6 >> 1);
}

ISR(TIMER0_COMPA_vect)
{
	Timer;			/* Performance counter for this module */
	disk_timerproc();	/* Drive timer procedure of low level disk I/O module */
}

ISR(__vector_default){}

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

int main(void)
{
  uint16_t i;
  uint8_t buffer[512];

	/* Start 100Hz system timer with TC0 */
	OCR0A = (F_CPU / 1024 / 100 - 1);
	TCCR0A = (1 << WGM01);
	TCCR0B = (1 << CS02 | 1 << CS00);
	// TCCR0B = (1 << CS02);
	TIMSK0 = (1 << OCIE0A);

	LED_DDR |= (1 << LED_PIN);

	DDRD |= (1 << PD4);
	DDRB &= ~(1 << MISO);
	DDRB |= (1 << MOSI) | (1 << SCK);

	sei();

  for(i = 0; i < 512; i++)
  {
    buffer[i] = i % 255;
  }

  fp = (FIL *) malloc(sizeof (FIL));
  fs = (FATFS *) malloc(sizeof(FATFS));

	uint8_t error = f_mount(fs, "", 1);

	ledFlicker(error);

  if (error == FR_OK)
	{
		f_open(fp, "write.txt", FA_WRITE | FA_OPEN_APPEND);
    for(i = 0; i < 255; i++)
    {
      f_write(fp, buffer, 512, &bw);
    }
  }



  return 0;
}
