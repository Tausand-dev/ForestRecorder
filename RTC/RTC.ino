#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#include <RTClibExtended.h>

#define SQW_PIN 2

#define SCHEDULE_LINE_LENGTH 32

RTC_DS3231 RTC;

char NEXT_TASK[SCHEDULE_LINE_LENGTH];
uint16_t NUMBER_TASKS = 0;
uint16_t DONE_TASKS = 0;

char RECORDING_NAME[15];

volatile boolean alarmIsrWasCalled = false;

void resetFunc(void)
{
    while (1);
}

void wakeUp()        // here the interrupt is handled after wakeup
{
    alarmIsrWasCalled = true;
//    RTC.armAlarm(1, false);
//    RTC.clearAlarm(1);
//    RTC.alarmInterrupt(1, false);
//
//    Serial.println("DUDE WAKE UP");
}

void setup() 
{
    Serial.begin(9600);
    Serial.println("Start communication");

    if (! RTC.begin())
    {
        Serial.println("Couldn't find RTC");
        resetFunc();
    }

    RTC.adjust(DateTime(__DATE__, __TIME__));

    RTC.armAlarm(1, false);
    RTC.clearAlarm(1);
    RTC.alarmInterrupt(1, false);
    RTC.armAlarm(2, false);
    RTC.clearAlarm(2);
    RTC.alarmInterrupt(2, false);

    pinMode(SQW_PIN, INPUT_PULLUP);
    attachInterrupt(INT0, wakeUp, FALLING);

    DateTime future (RTC.now() + TimeSpan(0,0,0,30));

    //byte seconds, byte minutes, byte hours, byte daydate
    Serial.print("Alarm will be triggered at: ");
    Serial.print(future.hour(), DEC);
    Serial.print(':');
    Serial.print(future.minute(), DEC);
    Serial.print(':');
    Serial.println(future.second(), DEC);
    RTC.setAlarm(ALM1_MATCH_HOURS, future.second(), future.minute(), future.hour(), 0);   //set your wake-up time here
    RTC.alarmInterrupt(1, true);
}

void loop()
{
    if (alarmIsrWasCalled)
    {
        Serial.print("Alarm Triggered: ");
        alarmIsrWasCalled = false;
    }
    else
    {
        DateTime now = RTC.now();
        
        Serial.print(now.year(), DEC);
        Serial.print('/');
        Serial.print(now.month(), DEC);
        Serial.print('/');
        Serial.print(now.day(), DEC);
        Serial.print(" ");
        Serial.print(now.hour(), DEC);
        Serial.print(':');
        Serial.print(now.minute(), DEC);
        Serial.print(':');
        Serial.print(now.second(), DEC);
        Serial.println();
        
        delay(1000);
    }
}
