#include "rtc.h"
#include "twi.h"
#include <avr/pgmspace.h>

// Macro to deal with the difference in I2C write functions from old and new Arduino versions.
static uint8_t read_i2c_register(uint8_t addr, uint8_t reg) {
  twi_begin_transmission(addr);
  twi_send_byte((uint8_t) reg);
  twi_end_transmission();

  twi_request_from(addr, (uint8_t)1);
  return twi_receive();
}

static void write_i2c_register(uint8_t addr, uint8_t reg, uint8_t val) {
  twi_begin_transmission(addr);
  twi_send_byte((uint8_t)reg);
  twi_send_byte((uint8_t)val);
  twi_end_transmission();
}

////////////////////////////////////////////////////////////////////////////////
// utility code, some of this could be exposed in the DateTime API if needed

const uint8_t daysInMonth [] PROGMEM = { 31,28,31,30,31,30,31,31,30,31,30,31 };

// number of days since 2000/01/01, valid for 2001..2099
static uint16_t date2days(uint16_t y, uint8_t m, uint8_t d)
{
    if (y >= 2000)
        y -= 2000;
    uint16_t days = d;
    for (uint8_t i = 1; i < m; ++i)
        days += pgm_read_byte(daysInMonth + i - 1);
    if (m > 2 && y % 4 == 0)
        ++days;
    return days + 365 * y + (y + 3) / 4 - 1;
}

static long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s)
{
    return ((days * 24L + h) * 60 + m) * 60 + s;
}

DateTime::DateTime (uint32_t t)
{
  t -= SECONDS_FROM_1970_TO_2000; // bring to 2000 timestamp from 1970
  ss = t % 60;
  t /= 60;
  mm = t % 60;
  t /= 60;
  hh = t % 24;
  uint16_t days = t / 24;
  uint8_t leap;
  for (yOff = 0; ; ++yOff)
  {
    leap = yOff % 4 == 0;
    if (days < (uint16_t) 365 + leap)
      break;
    days -= 365 + leap;
  }
  for (m = 1; ; ++m)
  {
    uint8_t daysPerMonth = pgm_read_byte(daysInMonth + m - 1);
    if (leap && m == 2)
      ++daysPerMonth;
    if (days < daysPerMonth)
      break;
    days -= daysPerMonth;
  }
  d = days + 1;
}

DateTime::DateTime (uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec)
{
    if (year >= 2000)
        year -= 2000;
    yOff = year;
    m = month;
    d = day;
    hh = hour;
    mm = min;
    ss = sec;
}

uint32_t DateTime::unixtime(void) const
{
  uint32_t t;
  uint16_t days = date2days(yOff, m, d);
  t = time2long(days, hh, mm, ss);
  t += SECONDS_FROM_1970_TO_2000;  // seconds from 1970 to 2000

  return t;
}

static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }


////////////////////////////////////////////////////////////////////////////////
// RTC_DS3231 implementation

bool RTC_DS3231::begin(void)
{
  twi_init_master();
  return true;
}

bool RTC_DS3231::lostPower(void)
{
  return (read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG) >> 7);
}

void RTC_DS3231::adjust(const DateTime& dt)
{
  twi_begin_transmission(DS3231_ADDRESS);
  twi_send_byte((uint8_t)0); // start at location 0
  twi_send_byte(bin2bcd(dt.second()));
  twi_send_byte(bin2bcd(dt.minute()));
  twi_send_byte(bin2bcd(dt.hour()));
  twi_send_byte(bin2bcd(0));
  twi_send_byte(bin2bcd(dt.day()));
  twi_send_byte(bin2bcd(dt.month()));
  twi_send_byte(bin2bcd(dt.year() - 2000));
  twi_end_transmission();

  uint8_t statreg = read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG);
  statreg &= ~0x80; // flip OSF bit
  write_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG, statreg);
}

DateTime RTC_DS3231::now()
{
  twi_begin_transmission(DS3231_ADDRESS);
  twi_send_byte((uint8_t)0);
  twi_end_transmission();

  twi_request_from(DS3231_ADDRESS, 7);
  uint8_t ss = bcd2bin(twi_receive() & 0x7F);
  uint8_t mm = bcd2bin(twi_receive());
  uint8_t hh = bcd2bin(twi_receive());
  twi_receive();
  uint8_t d = bcd2bin(twi_receive());
  uint8_t m = bcd2bin(twi_receive());
  uint16_t y = bcd2bin(twi_receive()) + 2000;

  return DateTime (y, m, d, hh, mm, ss);
}

Ds3231SqwPinMode RTC_DS3231::readSqwPinMode()
{
  int mode;

  twi_begin_transmission(DS3231_ADDRESS);
  twi_send_byte(DS3231_CONTROL);
  twi_end_transmission();

  twi_request_from((uint8_t)DS3231_ADDRESS, (uint8_t)1);
  mode = twi_receive();

  mode &= 0x93;
  return static_cast<Ds3231SqwPinMode>(mode);
}

void RTC_DS3231::writeSqwPinMode(Ds3231SqwPinMode mode)
{
  uint8_t ctrl;
  ctrl = read_i2c_register(DS3231_ADDRESS, DS3231_CONTROL);

  ctrl &= ~0x04; // turn off INTCON
  ctrl &= ~0x18; // set freq bits to 0

  if (mode == DS3231_OFF) {
    ctrl |= 0x04; // turn on INTCN
  } else {
    ctrl |= mode;
  }
  write_i2c_register(DS3231_ADDRESS, DS3231_CONTROL, ctrl);
}

float RTC_DS3231::getTemp()
{
  int8_t temp_msb, temp_lsb;

  twi_begin_transmission(DS3231_ADDRESS);
  twi_send_byte(DS3231_TEMP);
  twi_end_transmission();

  twi_request_from((uint8_t)DS3231_ADDRESS, (uint8_t)2);
  temp_msb = twi_receive();
  temp_lsb = twi_receive() >> 6;
  twi_end_transmission();

  return (float)(temp_msb & 0b01111111) + ((float)temp_lsb * 0.25);
}

/*----------------------------------------------------------------------*
 * Enable or disable an alarm "interrupt" which asserts the INT pin     *
 * on the RTC.                                                          *
 *----------------------------------------------------------------------*/
void RTC_DS3231::alarmInterrupt(uint8_t alarmNumber, bool interruptEnabled)
{
    uint8_t controlReg, mask;

    twi_begin_transmission(DS3231_ADDRESS);
    twi_send_byte(DS3231_CONTROL);
    controlReg = twi_end_transmission();
    if (!controlReg) {
      twi_request_from((uint8_t)DS3231_ADDRESS, (uint8_t)1);
      controlReg = twi_receive();
      twi_end_transmission();
    }

    mask = _BV(A1IE) << (alarmNumber - 1);
    if (interruptEnabled)
        controlReg |= mask;
    else
        controlReg &= ~mask;

    twi_begin_transmission(DS3231_ADDRESS);
    twi_send_byte(DS3231_CONTROL);
    twi_send_byte(controlReg);
    twi_end_transmission();
}

/*----------------------------------------------------------------------*
 * Set an alarm time. Sets the alarm registers only.  To cause the      *
 * INT pin to be asserted on alarm match, use alarmInterrupt().         *
 * This method can set either Alarm 1 or Alarm 2, depending on the      *
 * value of alarmType (use a value from the ALARM_TYPES_t enumeration). *
 * When setting Alarm 2, the seconds value must be supplied but is      *
 * ignored, recommend using zero. (Alarm 2 has no seconds register.)    *
 *----------------------------------------------------------------------*/
void RTC_DS3231::setAlarm(Ds3231_ALARM_TYPES_t alarmType, uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t daydate){

    uint8_t addr;
    uint8_t alarmNumber;

    seconds = bin2bcd(seconds);
    minutes = bin2bcd(minutes);
    hours = bin2bcd(hours);
    daydate = bin2bcd(daydate);
    if (alarmType & 0x01) seconds |= _BV(A1M1);
    if (alarmType & 0x02) minutes |= _BV(A1M2);
    if (alarmType & 0x04) hours |= _BV(A1M3);
    if (alarmType & 0x10) hours |= _BV(DYDT);
    if (alarmType & 0x08) daydate |= _BV(A1M4);

    if ( !(alarmType & 0x80) ) {    //alarm 1
        alarmNumber = 1;
        addr = ALM1_SECONDS;
        twi_begin_transmission(DS3231_ADDRESS);
        twi_send_byte(addr++);
        twi_send_byte(seconds);
        twi_end_transmission();
    }
    else {
        alarmNumber = 2;
        addr = ALM2_MINUTES;      //alarm 2
    }

    twi_begin_transmission(DS3231_ADDRESS);
    twi_send_byte(addr++);
    twi_send_byte(minutes);
    twi_end_transmission();

    twi_begin_transmission(DS3231_ADDRESS);
    twi_send_byte(addr++);
    twi_send_byte(hours);
    twi_end_transmission();

    twi_begin_transmission(DS3231_ADDRESS);
    twi_send_byte(addr++);
    twi_send_byte(daydate);
    twi_end_transmission();

    armAlarm(alarmNumber, true);
    clearAlarm(alarmNumber);
}

/*----------------------------------------------------------------------*
 * Set an alarm time. Sets the alarm registers only.  To cause the      *
 * INT pin to be asserted on alarm match, use alarmInterrupt().         *
 * This method can set either Alarm 1 or Alarm 2, depending on the      *
 * value of alarmType (use a value from the ALARM_TYPES_t enumeration). *
 * However, when using this method to set Alarm 1, the seconds value    *
 * is set to zero. (Alarm 2 has no seconds register.)                   *
 *----------------------------------------------------------------------*/
void RTC_DS3231::setAlarm(Ds3231_ALARM_TYPES_t alarmType, uint8_t minutes, uint8_t hours, uint8_t daydate) {
    setAlarm(alarmType, 0, minutes, hours, daydate);
}

/*----------------------------------------------------------------------*
 * This method arms or disarms Alarm 1 or Alarm 2, depending on the     *
 * value of alarmNumber (1 or 2) and arm (true or false).               *
 *----------------------------------------------------------------------*/
void RTC_DS3231::armAlarm(uint8_t alarmNumber, bool armed) {
    uint8_t value, mask;

    twi_begin_transmission(DS3231_ADDRESS);
    twi_send_byte(DS3231_CONTROL);
    twi_end_transmission();

    twi_request_from((uint8_t)DS3231_ADDRESS, (uint8_t)1);
    value = twi_receive();
    twi_end_transmission();

    mask = _BV(alarmNumber - 1);
    if (armed) {
        value |= mask;
    }
    else {
        value &= ~mask;
    }

    twi_begin_transmission(DS3231_ADDRESS);
    twi_send_byte(DS3231_CONTROL);
    twi_send_byte(value);
    twi_end_transmission();
}

/*----------------------------------------------------------------------*
 * This method clears the status register of Alarm 1 or Alarm 2,        *
 * depending on the value of alarmNumber (1 or 2).                      *
 *----------------------------------------------------------------------*/
void RTC_DS3231::clearAlarm(uint8_t alarmNumber) {
    uint8_t value, mask;

    twi_begin_transmission(DS3231_ADDRESS);
    twi_send_byte(DS3231_STATUSREG);
    twi_end_transmission();

    twi_request_from((uint8_t)DS3231_ADDRESS, (uint8_t)1);
    value = twi_receive();
    twi_end_transmission();

    mask = _BV(alarmNumber - 1);
    value &= ~mask;

    twi_begin_transmission(DS3231_ADDRESS);
    twi_send_byte(DS3231_STATUSREG);
    twi_send_byte(value);
    twi_end_transmission();
}

/*----------------------------------------------------------------------*
 * This method can check either Alarm 1 or Alarm 2, depending on the    *
 * value of alarmNumber (1 or 2).                                       *
 *----------------------------------------------------------------------*/
bool RTC_DS3231::isArmed(uint8_t alarmNumber) {
    uint8_t value;

    twi_begin_transmission(DS3231_ADDRESS);
    twi_send_byte(DS3231_CONTROL);
    twi_end_transmission();

    twi_request_from((uint8_t)DS3231_ADDRESS, (uint8_t)1);
    value = twi_receive();
    twi_end_transmission();

    if (alarmNumber == 1) {
      value &= 0b00000001;
    }
    else {
      value &= 0b00000010;
      value >>= 1;
    }
    return value;
}
