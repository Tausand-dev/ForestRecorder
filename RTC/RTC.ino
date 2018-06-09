#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

/*RTC*/
#include <RTClibExtended.h>

/*SD*/
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SdFat.h>

/*RTC*/
#define SQW_PIN 2
#define SCHEDULE_LINE_LENGTH 32
#define SCHEDULE_TIME1_INDEX 9
#define SCHEDULE_TIME2_INDEX 25
#define YEAR_I 0
#define MONTH_I 1
#define DAY_I 2
#define HOUR_I 3
#define MINUTE_I 4

RTC_DS3231 RTC;
uint8_t NEXT_START[5];
uint8_t NEXT_STOP[5];

char NEXT_TASK[SCHEDULE_LINE_LENGTH];
uint16_t NUMBER_TASKS = 0;
uint16_t DONE_TASKS = 0;

char RECORDING_NAME[15];
volatile boolean alarmIsrWasCalled = false;

/*SD*/
#define CARDCS 4

SdFat SD;

#define RESET_PIN A1

void resetFunc(void)
{
    digitalWrite(RESET_PIN, HIGH);
    delay(200);
    pinMode(RESET_PIN, OUTPUT);
    Serial.println("reset");
    delay(200);
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

    RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
    clearAlarms();

    pinMode(SQW_PIN, INPUT_PULLUP);
    attachInterrupt(INT0, wakeUp, FALLING);

    if (!SD.begin(CARDCS, SPI_FULL_SPEED))
    {
        Serial.println(F("SD failed, or not present"));
        resetFunc();
    }

    totalTasks();
    setNextTask();
    
    delay(1000);
    
    enterSleep();
}

void loop()
{
    if (alarmIsrWasCalled)
    {
        Serial.print("Alarm Triggered: ");
        
        DateTime now = RTC.now();
        uint16_t year = NEXT_START[YEAR_I] + 2000;
        alarmIsrWasCalled =  false;
        
        if ((NEXT_START[MONTH_I] != now.month()) | (year != now.year()))
        {   //deactivate
            enterSleep();
        }
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

        uint16_t year = NEXT_STOP[YEAR_I] + 2000;
        if ((year == now.year()) & (NEXT_STOP[MONTH_I] == now.month()) & (NEXT_STOP[DAY_I] == now.day()) & (NEXT_STOP[HOUR_I] == now.hour()) & (NEXT_STOP[MINUTE_I] == now.minute()))
        {
            Serial.println("STOP HERE");
            writeDoneTask(1);
            setNextTask();
            delay(1000);
            enterSleep();
        }
        
        delay(1000);
    }
}

void wakeUp() // here the interrupt is handled after wakeup
{
    alarmIsrWasCalled = true;
}

void enterSleep(void)
{
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    
    sleep_mode();
    sleep_disable();
    
    power_all_enable();
    delay(25);
}

void clearAlarms(void)
{
    RTC.armAlarm(1, false);
    RTC.clearAlarm(1);
    RTC.alarmInterrupt(1, false);
    RTC.armAlarm(2, false);
    RTC.clearAlarm(2);
    RTC.alarmInterrupt(2, false);
}

void setAlarm(byte date, byte hour, byte minute)
{
    Serial.print("Alarm will be triggered at: ");
    Serial.print(date, DEC);
    Serial.print(" ");
    Serial.print(hour, DEC);
    Serial.print(':');
    Serial.println(minute, DEC);
    
    //byte seconds, byte minutes, byte hours, byte daydate
    RTC.setAlarm(ALM1_MATCH_DATE, 0, minute, hour, date);   //set your wake-up time here
    RTC.alarmInterrupt(1, true);
}

uint16_t totalTasks(void)
{
    SdFile my_file;
    if(! my_file.open("schedule.dat", O_READ))
    {
        Serial.println(F("Schedule file does not exist."));
        resetFunc();
    }
    else
    {
        NUMBER_TASKS = my_file.fileSize() / SCHEDULE_LINE_LENGTH;
    }
    my_file.close();
    return NUMBER_TASKS;
}

void writeDoneTask(uint8_t write_next_task)
{
    SdFile my_file;
    if(my_file.open("done.dat", O_CREAT | O_WRITE | O_AT_END))
    {
        if (write_next_task & (NEXT_TASK[0] != ' '))
        {
            my_file.write(NEXT_TASK);
            my_file.println("");
            NEXT_TASK[0] = ' ';
        }
    }
    else
    {
        Serial.println("Error opening done.dat");
    }
    my_file.close();
}

uint16_t setDoneTasks(void)
{
    SdFile my_file;
    if(! my_file.open("done.dat", O_READ))
    {
        Serial.println("Done file does not exist.");
        DONE_TASKS = 0;
        writeDoneTask(0);
    }
    else
    {
        DONE_TASKS = my_file.fileSize() / SCHEDULE_LINE_LENGTH;
        Serial.print("DONE TASKS: ");
        Serial.println(DONE_TASKS, DEC);
    }
    
    my_file.close();
    return DONE_TASKS;
}

void setNextTask(void)
{
    char letter;
    uint16_t i, j;

    totalTasks();
    setDoneTasks();

    if (DONE_TASKS < NUMBER_TASKS)
    {
        SdFile my_file;
        if(! my_file.open("schedule.dat", O_READ))
        {
            Serial.println(F("Schedule file does not exist."));
            resetFunc();
        }
        else
        {
            for(i = 0; i < NUMBER_TASKS; i++)
            {
                for(j = 0; j < SCHEDULE_LINE_LENGTH; j++)
                {
                    letter = my_file.read();
                    if ((i == DONE_TASKS) & (j < SCHEDULE_LINE_LENGTH -1))
                    {
                        NEXT_TASK[j] = letter;
                    }
                }
            }
        }
        Serial.print("NEXT_TASK: ");
        Serial.write(NEXT_TASK);
        Serial.println("");
        my_file.close();
    
        taskToAlarm();
        taskToStop();
    }
    else
    {
        Serial.println("ALL DONE");
    }
}

void taskToAlarm(void)
{
    char temp[2];
    uint8_t day, month, hour, minute;
    uint16_t year;

    temp[0] = NEXT_TASK[0];
    temp[1] = NEXT_TASK[1];
    day = atoi(temp);

    temp[0] = NEXT_TASK[3];
    temp[1] = NEXT_TASK[4];
    month = atoi(temp);

    temp[0] = NEXT_TASK[6];
    temp[1] = NEXT_TASK[7];
    year = atoi(temp);

    temp[0] = NEXT_TASK[9];
    temp[1] = NEXT_TASK[10];
    hour = atoi(temp);
    
    temp[0] = NEXT_TASK[12];
    temp[1] = NEXT_TASK[13];
    minute = atoi(temp);

    setAlarm(day, hour, minute);

    NEXT_START[YEAR_I] = year;
    NEXT_START[MONTH_I] = month;
    NEXT_START[DAY_I] = day;
    NEXT_START[HOUR_I] = hour;
    NEXT_START[MINUTE_I] = minute;
}

void taskToStop(void)
{
    char temp[2];
    uint8_t day, month, hour, minute;
    uint16_t year;

    temp[0] = NEXT_TASK[0 + 16];
    temp[1] = NEXT_TASK[1 + 16];
    day = atoi(temp);

    temp[0] = NEXT_TASK[3 + 16];
    temp[1] = NEXT_TASK[4 + 16];
    month = atoi(temp);

    temp[0] = NEXT_TASK[6 + 16];
    temp[1] = NEXT_TASK[7 + 16];
    year = atoi(temp);

    temp[0] = NEXT_TASK[9 + 16];
    temp[1] = NEXT_TASK[10 + 16];
    hour = atoi(temp);
    
    temp[0] = NEXT_TASK[12 + 16];
    temp[1] = NEXT_TASK[13 + 16];
    minute = atoi(temp);
    
    NEXT_STOP[YEAR_I] = year;
    NEXT_STOP[MONTH_I] = month;
    NEXT_STOP[DAY_I] = day;
    NEXT_STOP[HOUR_I] = hour;
    NEXT_STOP[MINUTE_I] = minute;
}

