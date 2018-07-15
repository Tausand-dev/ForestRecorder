/*
 * DS RTC Library: DS1307 and DS3231 driver library
 * (C) 2011 Akafugu Corporation
 *
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 */

/*
 * DS3231 register map
 *
 *  00h-06h: seconds, minutes, hours, day-of-week, date, month, year (all in BCD)
 *       bit 7 should be set to zero: The DS3231 clock is always running
 *  07h: A1M1  Alarm 1 seconds
 *  08h: A1M2  Alarm 1 minutes
 *  09h: A1M3  Alarm 1 hour (bit6 is am/pm flag in 12h mode)
 *  0ah: A1M4  Alarm 1 day/date (bit6: 1 for day, 0 for date)
 *  0bh: A2M2  Alarm 2 minutes
 *  0ch: A2M3  Alarm 2 hour (bit6 is am/pm flag in 12h mode)
 *  0dh: A2M4  Alarm 2 day/data (bit6: 1 for day, 0 for date)
 *       <see data sheet page12 for Alarm register mask bit tables:
 *        for alarm when hours, minutes and seconds match set 1000 for alarm 1>
 *  0eh: control
 *      bit7: !EOSC
 *      bit6: BBSQW
 *      bit5: CONV
 *      bit4: RS2
 *      bit3: RS1
 *      bit2: INTCN
 *      bit1: A2IE
 *      bit0: A1IE
 *  0fh: control/status
 *      bit7: OSF
 *      bit6: 0
 *      bit5: 0
 *      bit4: 0
 *      bit3: EN32kHz
 *      bit2: BSY
 *      bit1: A2F alarm 2 flag
 *      bit0: A1F alarm 1 flag
 * 10h: aging offset (signed)
 * 11h: MSB of temp (signed)
 * 12h: LSB of temp in bits 7 and 6 (0.25 degrees for each 00, 01, 10, 11)
 *
 */

#include <avr/io.h>
#include "rtc.h"

#define RTC_ADDR 0x68 // I2C address
#define CH_BIT 7 // clock halt bit

// statically allocated structure for time value
struct tm _tm;

uint8_t dec2bcd(uint8_t d)
{
  return ((d/10 * 16) + (d % 10));
}

uint8_t bcd2dec(uint8_t b)
{
  return ((b/16 * 10) + (b % 16));
}

uint8_t rtc_read_byte(uint8_t offset)
{
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(offset);
	twi_end_transmission();

	twi_request_from(RTC_ADDR, 1);
	return twi_receive();
}

void rtc_write_byte(uint8_t b, uint8_t offset)
{
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(offset);
	twi_send_byte(b);
	twi_end_transmission();
}


void rtc_init(void)
{
	// Attempt autodetection:
	// 1) Read and save temperature register
	// 2) Write a value to temperature register
	// 3) Read back the value
	//   equal to the one written: DS1307, write back saved value and return
	//   different from written:   DS3231
	rtc_write_byte(0xee, 0x11);
	rtc_write_byte(0xdd, 0x12);
}

struct tm* rtc_get_time(void)
{
	uint8_t rtc[9];
	uint8_t century = 0;

	// read 7 bytes starting from register 0
	// sec, min, hour, day-of-week, date, month, year
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(0x0);
	twi_end_transmission();

	twi_request_from(RTC_ADDR, 7);

  uint8_t i;
	for (i = 0; i < 7; i++) {
		rtc[i] = twi_receive();
	}

	twi_end_transmission();

	// Clear clock halt bit from read data
	// This starts the clock for a DS1307, and has no effect for a DS3231
	rtc[0] &= ~(_BV(CH_BIT)); // clear bit

	_tm.sec = bcd2dec(rtc[0]);
	_tm.min = bcd2dec(rtc[1]);
	_tm.hour = bcd2dec(rtc[2]);
	_tm.mday = bcd2dec(rtc[4]);
	_tm.mon = bcd2dec(rtc[5] & 0x1F); // returns 1-12
	century = (rtc[5] & 0x80) >> 7;
	_tm.year = century == 1 ? 2000 + bcd2dec(rtc[6]) : 1900 + bcd2dec(rtc[6]); // year 0-99
	_tm.wday = bcd2dec(rtc[3]); // returns 1-7

	if (_tm.hour == 0) {
		_tm.twelveHour = 0;
		_tm.am = 1;
	} else if (_tm.hour < 12) {
		_tm.twelveHour = _tm.hour;
		_tm.am = 1;
	} else {
		_tm.twelveHour = _tm.hour - 12;
		_tm.am = 0;
	}

	return &_tm;
}

void rtc_get_time_s(uint8_t* hour, uint8_t* min, uint8_t* sec)
{
	uint8_t rtc[9];

	// read 7 bytes starting from register 0
	// sec, min, hour, day-of-week, date, month, year
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(0x0);
	twi_end_transmission();

	twi_request_from(RTC_ADDR, 7);
  uint8_t i;
	for(i=0; i<7; i++) {
		rtc[i] = twi_receive();
	}

	twi_end_transmission();

	if (sec)  *sec =  bcd2dec(rtc[0]);
	if (min)  *min =  bcd2dec(rtc[1]);
	if (hour) *hour = bcd2dec(rtc[2]);
}

// fixme: support 12-hour mode for setting time
void rtc_set_time(struct tm* tm_)
{
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(0x0);

	uint8_t century;
	if (tm_->year > 2000) {
		century = 0x80;
		tm_->year = tm_->year - 2000;
	} else {
		century = 0;
		tm_->year = tm_->year - 1900;
	}

	// clock halt bit is 7th bit of seconds: this is always cleared to start the clock
	twi_send_byte(dec2bcd(tm_->sec)); // seconds
	twi_send_byte(dec2bcd(tm_->min)); // minutes
	twi_send_byte(dec2bcd(tm_->hour)); // hours
	twi_send_byte(dec2bcd(tm_->wday)); // day of week
	twi_send_byte(dec2bcd(tm_->mday)); // day
	twi_send_byte(dec2bcd(tm_->mon) + century); // month
	twi_send_byte(dec2bcd(tm_->year)); // year

	twi_end_transmission();
}

void rtc_set_time_s(uint8_t hour, uint8_t min, uint8_t sec)
{
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(0x0);

	// clock halt bit is 7th bit of seconds: this is always cleared to start the clock
	twi_send_byte(dec2bcd(sec)); // seconds
	twi_send_byte(dec2bcd(min)); // minutes
	twi_send_byte(dec2bcd(hour)); // hours

	twi_end_transmission();
}

// DS1307 only (has no effect when run on DS3231)
// halt/start the clock
// 7th bit of register 0 (second register)
// 0 = clock is running
// 1 = clock is not running
void rtc_run_clock(bool run)
{
  return;
}

bool rtc_is_clock_running(void)
{
  return true;
}

void ds3231_get_temp_int(int8_t* i, uint8_t* f)
{
	uint8_t msb, lsb;

	*i = 0;
	*f = 0;

	twi_begin_transmission(RTC_ADDR);
	// temp registers 0x11 and 0x12
	twi_send_byte(0x11);
	twi_end_transmission();

	twi_request_from(RTC_ADDR, 2);

	if (twi_available()) {
		msb = twi_receive(); // integer part (in twos complement)
		lsb = twi_receive(); // fraction part

		// integer part in entire byte
		*i = msb;
		// fractional part in top two bits (increments of 0.25)
		*f = (lsb >> 6) * 25;

		// float value can be read like so:
		// float temp = ((((short)msb << 8) | (short)lsb) >> 6) / 4.0f;
	}
}

void rtc_force_temp_conversion(uint8_t block)
{
	// read control register (0x0E)
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(0x0E);
	twi_end_transmission();

	twi_request_from(RTC_ADDR, 1);
	uint8_t ctrl = twi_receive();

	ctrl |= 0b00100000; // Set CONV bit

	// write new control register value
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(0x0E);
	twi_send_byte(ctrl);
	twi_end_transmission();

	if (!block) return;

	// Temp conversion is ready when control register becomes 0
	do {
		// Block until CONV is 0
		twi_begin_transmission(RTC_ADDR);
		twi_send_byte(0x0E);
		twi_end_transmission();
		twi_request_from(RTC_ADDR, 1);
	} while ((twi_receive() & 0b00100000) != 0);
}

// Alarm functionality
// fixme: should decide if "alarm disabled" mode should be available, or if alarm should always be enabled
// at 00:00:00. Currently, "alarm disabled" only works for ds3231
void rtc_reset_alarm(void)
{
  rtc_write_byte(0, 0x07); // second
	rtc_write_byte(0, 0x08); // minute
	rtc_write_byte(0, 0x09); // hour
	rtc_write_byte(0, 0x0a); // day
}

// fixme: add an option to set whether or not the INTCN and Interrupt Enable flag is set when setting the alarm
void rtc_set_alarm_s(uint8_t hour, uint8_t min, uint8_t sec)
{
	if (hour > 23) return;
	if (min > 59) return;
	if (sec > 59) return;
	/*
	 *  07h: A1M1:0  Alarm 1 seconds
	 *  08h: A1M2:0  Alarm 1 minutes
	 *  09h: A1M3:0  Alarm 1 hour (bit6 is am/pm flag in 12h mode)
	 *  0ah: A1M4:1  Alarm 1 day/date (bit6: 1 for day, 0 for date)
	 *  Sets alarm to fire when hour, minute and second matches
	 */
	rtc_write_byte(dec2bcd(sec),  0x07); // second
	rtc_write_byte(dec2bcd(min),  0x08); // minute
	rtc_write_byte(dec2bcd(hour), 0x09); // hour
	rtc_write_byte(0b10000001,         0x0a); // day (upper bit must be set)

	// clear alarm flag
	uint8_t val = rtc_read_byte(0x0f);
	rtc_write_byte(val & ~0b00000001, 0x0f);
}

void rtc_set_alarm(struct tm* tm_)
{
	if (!tm_) return;
	rtc_set_alarm_s(tm_->hour, tm_->min, tm_->sec);
}

void rtc_get_alarm_s(uint8_t* hour, uint8_t* min, uint8_t* sec)
{
	*sec  = bcd2dec(rtc_read_byte(0x07) & ~0b10000000);
	*min  = bcd2dec(rtc_read_byte(0x08) & ~0b10000000);
	*hour = bcd2dec(rtc_read_byte(0x09) & ~0b10000000);
}

struct tm* rtc_get_alarm(void)
{
	uint8_t hour, min, sec;

	rtc_get_alarm_s(&hour, &min, &sec);
	_tm.hour = hour;
	_tm.min = min;
	_tm.sec = sec;
	return &_tm;
}

bool rtc_check_alarm(void)
{
	// Alarm 1 flag (A1F) in bit 0
	uint8_t val = rtc_read_byte(0x0f);

	// clear flag when set
	if (val & 1)
		rtc_write_byte(val & ~0b00000001, 0x0f);

	return val & 1 ? 1 : 0;
}
