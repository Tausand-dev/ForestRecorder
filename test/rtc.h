// Code by JeeLabs http://news.jeelabs.org/code/
// Extended by Fabio Cuomo https://github.com/FabioCuomo/FabioCuomo-DS3231/
// Released to the public domain! Enjoy!

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define DS3231_ADDRESS               0x68
#define DS3231_CONTROL               0x0E
#define DS3231_STATUSREG             0x0F
#define DS3231_TEMP                  0x11

#define SECONDS_PER_DAY              86400L

#define SECONDS_FROM_1970_TO_2000    946684800

//Control register bits
#define A1IE 0
#define A2IE 1

//Alarm mask bits
#define A1M1 7
#define A1M2 7
#define A1M3 7
#define A1M4 7
#define A2M2 7
#define A2M3 7
#define A2M4 7

//DS3232 Register Addresses
#define ALM1_SECONDS 0x07
#define ALM1_MINUTES 0x08
#define ALM1_HOURS 0x09
#define ALM1_DAYDATE 0x0A
#define ALM2_MINUTES 0x0B
#define ALM2_HOURS 0x0C
#define ALM2_DAYDATE 0x0D

//Other
#define DYDT 6                     //Day/Date flag bit in alarm Day/Date registers

// Simple general-purpose date/time class (no TZ / DST / leap second handling!)
class DateTime
{
  public:
    DateTime (uint32_t t =0);
    DateTime (uint16_t year, uint8_t month, uint8_t day,
                uint8_t hour =0, uint8_t min =0, uint8_t sec =0);
    uint16_t year() const       { return 2000 + yOff; }
    uint8_t month() const       { return m; }
    uint8_t day() const         { return d; }
    uint8_t hour() const        { return hh; }
    uint8_t minute() const      { return mm; }
    uint8_t second() const      { return ss; }

    // 32-bit times as seconds since 1/1/1970
    uint32_t unixtime(void) const;

  protected:
    uint8_t yOff, m, d, hh, mm, ss;
};

// RTC based on the DS3231 chip connected via I2C and the Wire library
enum Ds3231SqwPinMode { DS3231_OFF = 0x01, DS3231_SquareWave1Hz = 0x00, DS3231_SquareWave1kHz = 0x08, DS3231_SquareWave4kHz = 0x10, DS3231_SquareWave8kHz = 0x18 };

//Alarm masks
enum Ds3231_ALARM_TYPES_t {
    ALM1_EVERY_SECOND = 0x0F,
    ALM1_MATCH_SECONDS = 0x0E,
    ALM1_MATCH_MINUTES = 0x0C,     //match minutes *and* seconds
    ALM1_MATCH_HOURS = 0x08,       //match hours *and* minutes, seconds
    ALM1_MATCH_DATE = 0x00,        //match date *and* hours, minutes, seconds
    ALM1_MATCH_DAY = 0x10,         //match day *and* hours, minutes, seconds

    ALM2_EVERY_MINUTE = 0x8E,
    ALM2_MATCH_MINUTES = 0x8C,     //match minutes
    ALM2_MATCH_HOURS = 0x88,       //match hours *and* minutes
    ALM2_MATCH_DATE = 0x80,        //match date *and* hours, minutes
    ALM2_MATCH_DAY = 0x90,         //match day *and* hours, minutes
};

class RTC_DS3231
{
  public:
    bool begin(void);
    static void adjust(const DateTime& dt);
    bool lostPower(void);
    static DateTime now();
    static Ds3231SqwPinMode readSqwPinMode();
    static void writeSqwPinMode(Ds3231SqwPinMode mode);
    float getTemp();
    void setAlarm(Ds3231_ALARM_TYPES_t alarmType, uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t daydate);
    void setAlarm(Ds3231_ALARM_TYPES_t alarmType, uint8_t minutes, uint8_t hours, uint8_t daydate);
    void armAlarm(uint8_t alarmNumber, bool armed);
    void alarmInterrupt(uint8_t alarmNumber, bool alarmEnabled);
    bool isArmed(uint8_t alarmNumber);
    void clearAlarm(uint8_t alarmNumber);
};
