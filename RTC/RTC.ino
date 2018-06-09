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

RTC_DS3231 RTC;

char NEXT_TASK[SCHEDULE_LINE_LENGTH];
uint16_t NUMBER_TASKS = 0;
uint16_t DONE_TASKS = 0;

char RECORDING_NAME[15];
volatile boolean alarmIsrWasCalled = false;

String readString; //main captured String 
String angle; //data String
String fuel;
String speed1;
String altidude;

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

void wakeUp()        // here the interrupt is handled after wakeup
{
    alarmIsrWasCalled = true;
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
    clearAlarms();

    pinMode(SQW_PIN, INPUT_PULLUP);
    attachInterrupt(INT0, wakeUp, FALLING);

//    DateTime future (RTC.now() + TimeSpan(0,0,1,0));
//    setAlarm(future.hour(), future.minute());

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

//void writeDoneTask(uint8_t write_next_task)
//{
//    File done_file = SD.open("/done.dat", FILE_WRITE);
//    if(done_file) 
//    {
//        if (write_next_task & (NEXT_TASK[0] != ' '))
//        {
//            done_file.write(NEXT_TASK);
//            done_file.println("");
//            NEXT_TASK[0] = ' ';
//        }
//    }
//    else
//    {
//        Serial.println("Error opening done.dat");
//    }
//    done_file.close();
//}

//uint16_t setDoneTasks(void)
//{
//    File done_file = SD.open("/done.dat");
//    if(! done_file)
//    {
//        Serial.println("Done file does not exist.");
//        DONE_TASKS = 0;
//        writeDoneTask(0);
//    }
//    else
//    {
//        DONE_TASKS = done_file.size() / SCHEDULE_LINE_LENGTH;
//        Serial.print("DONE TASKS: ");
//        Serial.println(DONE_TASKS, DEC);
//    }
//    done_file.close();
//    return DONE_TASKS;
//}

void setNextTask(void)
{
    char letter;
    uint16_t i, j;

    totalTasks();
//    setDoneTasks();

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
}

void taskToAlarm(void)
{
    char temp[2];
    byte day, month, hour, minute;

    temp[0] = NEXT_TASK[0];
    temp[1] = NEXT_TASK[1];
    day = atoi(temp);

    temp[0] = NEXT_TASK[3];
    temp[1] = NEXT_TASK[4];
    month = atoi(temp);

    temp[0] = NEXT_TASK[9];
    temp[1] = NEXT_TASK[10];
    hour = atoi(temp);
    
    temp[0] = NEXT_TASK[12];
    temp[1] = NEXT_TASK[13];
    minute = atoi(temp);

    setAlarm(day, hour, minute);
}
