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

/*RECORDER*/
#define BREAKOUT_RESET  9      // VS1053 reset pin (output)
#define BREAKOUT_CS     10     // VS1053 chip select pin (output)
#define BREAKOUT_DCS    8      // VS1053 Data/command select pin (output)
// These are common pins between breakout and shield
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

#define RECBUFFSIZE 128  // 64 or 128 bytes.
#define MWORDS 256
#define MBYTES (MWORDS * 2)

uint8_t IS_RECORDING = false;
uint8_t RECORDING_BUFFER[RECBUFFSIZE];
Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(BREAKOUT_RESET, BREAKOUT_CS, BREAKOUT_DCS, DREQ, CARDCS);

SdFile RECORDING_FILE;


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

    if (! musicPlayer.begin())
    {
        Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
        resetFunc();
    }
    
    if (! musicPlayer.prepareRecordOgg("v44k1q05.img"))
    {
         Serial.println("Couldn't load plugin!");
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
        {   
            // DEACTIVATE
            enterSleep();
        }
        else
        {
            IS_RECORDING = true;
            recordingFunc(false);
        }
    }
    else
    {
        recordingFunc(false);
        
        DateTime now = RTC.now();
        
//        Serial.print(now.year(), DEC);
//        Serial.print('/');
//        Serial.print(now.month(), DEC);
//        Serial.print('/');
//        Serial.print(now.day(), DEC);
//        Serial.print(" ");
//        Serial.print(now.hour(), DEC);
//        Serial.print(':');
//        Serial.print(now.minute(), DEC);
//        Serial.print(':');
//        Serial.print(now.second(), DEC);
//        Serial.println();

        uint16_t year = NEXT_STOP[YEAR_I] + 2000;
        if ((year == now.year()) & (NEXT_STOP[MONTH_I] == now.month()) & (NEXT_STOP[DAY_I] == now.day()) & (NEXT_STOP[HOUR_I] == now.hour()) & (NEXT_STOP[MINUTE_I] == now.minute()))
        {
            Serial.println("STOP HERE");
            recordingFunc(true);
            writeDoneTask(1);
            setNextTask();
            
            delay(1000);
            enterSleep();
        }
        
//        delay(1000);
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

void recordingFunc(bool aboutToChange)
{
    if (!IS_RECORDING && !aboutToChange)
    {
        Serial.println("Begin recording");
        IS_RECORDING = true;

        strcpy(RECORDING_NAME, "Record00.ogg");
        for (uint8_t i = 0; i < 100; i++)
        {
            RECORDING_NAME[6] = '0' + i/10;
            RECORDING_NAME[7] = '0' + i%10;
            // create if does not exist, do not open existing, write, sync after write
            if (! SD.exists(RECORDING_NAME))
            {
                break;
            }
        }
        Serial.print("Recording to "); 
        Serial.println(RECORDING_NAME);

        if (! RECORDING_FILE.open(RECORDING_NAME, O_CREAT | O_WRITE | O_AT_END))
        {
             Serial.println("Couldn't open file to record!");
             resetFunc();
        }
        
        musicPlayer.startRecordOgg(true); // use microphone (for linein, pass in 'false')
    }
    if (IS_RECORDING)
    {
        saveRecordedData(IS_RECORDING);
    }
    if (IS_RECORDING && aboutToChange)
    {
        Serial.println("End recording");
        musicPlayer.stopRecordOgg();
        IS_RECORDING = false;
        saveRecordedData(IS_RECORDING);
        
        RECORDING_FILE.close();

        while (SD.card()->isBusy())
        {
            Serial.println("SD BUSY");
        }
        Serial.println("DONE");
    }
}

uint16_t saveRecordedData(boolean isrecord)
{
    uint16_t written = 0;
    // read how many words are waiting for us
    uint16_t wordswaiting = musicPlayer.recordedWordsWaiting();    
    // try to process 256 words (512 bytes) at a time, for best speed
    while (wordswaiting > MWORDS)
    {
//        Serial.print("Waiting: "); Serial.println(wordswaiting);
        // for example 128 bytes x 4 loops = 512 bytes
        for (int x = 0; x < MBYTES/RECBUFFSIZE; x++)
        {
        // fill the buffer!
            for (uint16_t addr = 0; addr < RECBUFFSIZE; addr+=2)
            {
                uint16_t t = musicPlayer.recordedReadWord();
                //Serial.println(t, HEX);
                RECORDING_BUFFER[addr] = t >> 8; 
                RECORDING_BUFFER[addr+1] = t;
            }
            if (! RECORDING_FILE.write(RECORDING_BUFFER, RECBUFFSIZE))
            {
                Serial.print("Couldn't write "); Serial.println(RECBUFFSIZE); 
                resetFunc();
            }
        }
        // flush 512 bytes at a time
        RECORDING_FILE.flush();
        written += MWORDS;
        wordswaiting -= MWORDS;
    }
    wordswaiting = musicPlayer.recordedWordsWaiting();
    if (!isrecord)
    {
        Serial.print(wordswaiting); Serial.println(" remaining");
        // wrapping up the recording!
        uint16_t addr = 0;
        for (int x=0; x < wordswaiting-1; x++)
        {
            // fill the buffer!
            uint16_t t = musicPlayer.recordedReadWord();
            RECORDING_BUFFER[addr] = t >> 8; 
            RECORDING_BUFFER[addr+1] = t;
            if (addr > RECBUFFSIZE)
            {
                if (! RECORDING_FILE.write(RECORDING_BUFFER, RECBUFFSIZE))
                {
                    Serial.println("Couldn't write!");
                    resetFunc();
                }
                RECORDING_FILE.flush();
                addr = 0;
            }
        }
        if (addr != 0)
        {
            if (! RECORDING_FILE.write(RECORDING_BUFFER, addr))
            {
                Serial.println("Couldn't write!");
                resetFunc();
            }
            RECORDING_FILE.flush();
            written += addr;
        }
        musicPlayer.sciRead(VS1053_SCI_AICTRL3);
        
        if (! (musicPlayer.sciRead(VS1053_SCI_AICTRL3) & _BV(2)))
        {
            RECORDING_FILE.write(musicPlayer.recordedReadWord() & 0xFF);
            written++;
        }
        RECORDING_FILE.flush();
    }
    return written;
}

