/*************************************************** 
  This is an example for the Adafruit VS1053 Codec Breakout

  Designed specifically to work with the Adafruit VS1053 Codec Breakout 
  ----> https://www.adafruit.com/products/1381

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

// These are the pins used for the breakout example
#define BREAKOUT_RESET  9      // VS1053 reset pin (output)
#define BREAKOUT_CS     10     // VS1053 chip select pin (output)
#define BREAKOUT_DCS    8      // VS1053 Data/command select pin (output)
// These are common pins between breakout and shield
#define CARDCS 4     // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

#define SCHEDULE_LINE_LENGTH 32

//Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(BREAKOUT_RESET, BREAKOUT_CS, BREAKOUT_DCS, DREQ, CARDCS);

char NEXT_TASK[SCHEDULE_LINE_LENGTH];
uint16_t NUMBER_TASKS = 0;
uint16_t DONE_TASKS = 0;

void setup() 
{
    Serial.begin(115200);
    Serial.println("Start communication");
    setupWatchDogTimer();
//    if (! musicPlayer.begin())
//    {
//        Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
//        while (1);
//    }
//    
//    Serial.println(F("VS1053 found"));
//    if (!SD.begin(CARDCS))
//    {
//        Serial.println(F("SD failed, or not present"));
//        while (1);  // don't do anything more
//    }
//
//    totalTasks();
//    Serial.print("TOTAL TASKS: ");
//    Serial.println(NUMBER_TASKS, DEC);

    delay(10000);
}

void loop()
{
      enterSleep();
      doTask();
}

void doTask()
{
    //setNextTask();
    delay(2000);
    //writeDoneTask(1);
    delay(25);
}

ISR(WDT_vect)
{
    
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

void setupWatchDogTimer()
{
    
    MCUSR &= ~(1<<WDRF);

    // Set the WDCE bit (bit 4) and the WDE bit (bit 3) of the WDTCSR. The WDCE
    // bit must be set in order to change WDE or the watchdog pre-scalers.
    // Setting the WDCE bit will allow updates to the pre-scalers and WDE for 4
    // clock cycles then it will be reset by hardware.
    WDTCSR |= (1<<WDCE) | (1<<WDE);

    /**
     *  Setting the watchdog pre-scaler value with VCC = 5.0V and 16mHZ
     *  WDP3 WDP2 WDP1 WDP0 | Number of WDT | Typical Time-out at Oscillator Cycles
     *  0    0    0    0    |   2K cycles   | 16 ms
     *  0    0    0    1    |   4K cycles   | 32 ms
     *  0    0    1    0    |   8K cycles   | 64 ms
     *  0    0    1    1    |  16K cycles   | 0.125 s
     *  0    1    0    0    |  32K cycles   | 0.25 s
     *  0    1    0    1    |  64K cycles   | 0.5 s
     *  0    1    1    0    |  128K cycles  | 1.0 s
     *  0    1    1    1    |  256K cycles  | 2.0 s
     *  1    0    0    0    |  512K cycles  | 4.0 s
     *  1    0    0    1    | 1024K cycles  | 8.0 s
    */
    WDTCSR  = (1 << WDP3) | (0 << WDP2) | (0 << WDP1) | (1 << WDP0);
    // Enable the WD interrupt (note: no reset).
    WDTCSR |= _BV(WDIE);
}

uint16_t totalTasks(void)
{
    File schedule_file = SD.open("/schedule.dat");
    
    if(! schedule_file)
    {
        Serial.println("Schedule file does not exist.");
    }
    else
    {
        NUMBER_TASKS = schedule_file.size() / SCHEDULE_LINE_LENGTH;
    }
    schedule_file.close();
    return NUMBER_TASKS;
}

void writeDoneTask(uint8_t write_next_task)
{
    File done_file = SD.open("/done.dat", FILE_WRITE);
    if(done_file) 
    {
        if (write_next_task & (NEXT_TASK[0] != ' '))
        {
            done_file.write(NEXT_TASK);
            done_file.println("");
            NEXT_TASK[0] = ' ';
        }
    }
    else
    {
        Serial.println("Error opening done.dat");
    }
    done_file.close();
}

uint16_t setDoneTasks(void)
{
    File done_file = SD.open("/done.dat");
    if(! done_file)
    {
        Serial.println("Done file does not exist.");
        DONE_TASKS = 0;
        writeDoneTask(0);
    }
    else
    {
        DONE_TASKS = done_file.size() / SCHEDULE_LINE_LENGTH;
        Serial.print("DONE TASKS: ");
        Serial.println(DONE_TASKS, DEC);
    }
    done_file.close();
    return DONE_TASKS;
}

void setNextTask(void)
{
    char letter;
    uint16_t i, j;

    totalTasks();
    setDoneTasks();

    File schedule_file = SD.open("/schedule.dat");
    
    if(! schedule_file)
    {
        Serial.println("Schedule file does not exist.");
    }
    else
    {
        for(i = 0; i < NUMBER_TASKS; i++)
        {
            for(j = 0; j < SCHEDULE_LINE_LENGTH; j++)
            {
                letter = schedule_file.read();
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
    schedule_file.close();
}

void printDirectory(File dir, int numTabs)
{
    while(true)
    {
        File entry =  dir.openNextFile();
        if(! entry)
        {
            Serial.println("**nomorefiles**");
            break;
        }
        for (uint8_t i=0; i<numTabs; i++) 
        {
            Serial.print('\t');
        }
        Serial.print(entry.name());
        if(entry.isDirectory())
        {
            Serial.println("/");
            printDirectory(entry, numTabs+1);
        }
        else
        {
            Serial.print("\t\t");
            Serial.println(entry.size(), DEC);
        }
        entry.close();
    }
}
