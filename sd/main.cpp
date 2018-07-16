/*
    fpe:       board128.cpp
    Version:    0.1.0
    Date:       Feb. 21, 2013
	License:	GPL v2

	atmega128 board code

    ****************************************************************************
    Copyright (C) 2013 Radu Motisan  <radu.motisan@gmail.com>

	http://www.pocketmagic.net

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    ****************************************************************************
 */

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include "sdcard/ff.h"
#include "sdcard/integer.h"
#include "serial.h"

FATFS FatFs;	// FatFs work area
FIL *fp; // fpe object
UART serial(57600);

/*---------------------------------------------------------*/
/* User Provided RTC Function called by FatFs module       */
/* Used to provide a Timestamp for SDCard files and folders*/
DWORD get_fattime (void)
{
	// Returns current time packed into a DWORD variable
	return	  ((DWORD)(2013 - 1980) << 25)	// Year 2013
	| ((DWORD)8 << 21)				// Month 7
	| ((DWORD)2 << 16)				// Mday 28
	| ((DWORD)20 << 11)				// Hour 0..24
	| ((DWORD)30 << 5)				// Min 0
	| ((DWORD)0 >> 1);				// Sec 0
}

void SPI_MasterInit(void)
{
	/* Set MOSI and SCK output, all others input */
	DDRB = (1 << DDB3)|(1 << DDB5);
	DDRB |= (1 << PORTB1); // CS on arduino pin 9
	/* Enable SPI, Master, set clock rate fck/128 */
	SPCR = (1 << SPE)|(1 << MSTR)|(1 << SPR0)|(1 << SPR1);

}

int main(void)
{
	// init sdcard
	serial.println("Start");
	UINT bw;
	f_mount(0, &FatFs);		// Give a work area to the FatFs module
	serial.println("F_mount");
	// open file
	fp = (FIL *)malloc(sizeof (FIL));
	uint8_t temp;

	temp = f_open(fp, "file.txt", FA_WRITE | FA_CREATE_ALWAYS);

	if (temp == FR_OK)
	{	// Create a file
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


	// test append
	// if (f_open(fp, "file.txt", FA_WRITE | FA_OPEN_ALWAYS) == FR_OK)
	// {	// Open existing or create new file
	// 	if (f_lseek(fp, f_size(fp)) == FR_OK)
	// 	{
	// 		const char *text2 = "This is a new line, appended to existing file!\r\n";
	// 		f_write(fp, text2, strlen(text2), &bw);	// Write data to the file
	// 	}
	// 	f_close(fp);// Close the file
	// }
 	// char str[12];


	// get card volume
	// char szCardLabel[12] = {0};
	// DWORD sn = 0;
	// if (f_getlabel("", szCardLabel, &sn) == FR_OK) {
	// 	lcd.send_format_string("%s SN:%X\n", szCardLabel, sn);
	// }
	//
	// // read from file
	// if (f_open(fp, "file.txt", FA_READ ) == FR_OK) {	// Create a file
	// 	char text[255];
	// 	UINT br;
	// 	f_read(fp, text, 255, &br);
	// 	f_close(fp);// Close the file
	// 	// cut text the easy way
	// 	text[10] = 0;
	// 	lcd.send_format_string("Read:%s", text);
	// }
	//
	//
	// // signaling led
	// led2.Set(1);
	//
	// // loop
	// int i = 0;
	// while(1)
	// {
	// 	i++;
	// 	led2.Toggle(); // just signal loop
	// 	lcd.goto_xy(13,0);
	// 	lcd.send_format_string("%d ", i%10);
	// 	_delay_ms(1000);
	// }
}
