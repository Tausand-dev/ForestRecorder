



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
//#include <SD.h>
#include <BlockDriver.h>
#include <FreeStack.h>
#include <MinimumSerial.h>
#include <SdFat.h>
#include <SdFatConfig.h>
#include <SysCall.h>


#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

// These are the pins used for the breakout example
#define BREAKOUT_RESET  9      // VS1053 reset pin (output)
#define BREAKOUT_CS     10     // VS1053 chip select pin (output)
#define BREAKOUT_DCS    8      // VS1053 Data/command select pin (output)
// These are common pins between breakout and shield
#define CARDCS 2     // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

#define RECBUFFSIZE 64  // 64 or 128 bytes.

#define MWORDS 256
#define MBYTES (MWORDS * 2)

#define SCHEDULE_LINE_LENGTH 32

//File recording;  // the file we will save our recording to
uint8_t recording_buffer[RECBUFFSIZE];
Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(BREAKOUT_RESET, BREAKOUT_CS, BREAKOUT_DCS, DREQ, CARDCS);


char NEXT_TASK[SCHEDULE_LINE_LENGTH];
uint16_t NUMBER_TASKS = 0;
uint16_t DONE_TASKS = 0;

uint8_t aboutToChange = false;
uint8_t isRecording = false;

//void(* resetFunc) (void) = 0;

SdFat SD;
SdFile recording;

char RECORDING_NAME[15];

void resetFunc(void)
{
    while (1);
}

void setup() 
{
    Serial.begin(9600);
    Serial.println("Start communication");
    
    if (! musicPlayer.begin())
    {
        Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
        resetFunc();
    }
    
    Serial.println(F("VS1053 found"));
    if (!SD.begin(CARDCS, SPI_FULL_SPEED))
    {
        Serial.println(F("SD failed, or not present"));
        resetFunc();
    }
    
    //totalTasks();
//    Serial.print("TOTAL TASKS: ");
//    Serial.println(NUMBER_TASKS, DEC);

    if (! musicPlayer.prepareRecordOgg("v44k1q05.img"))
    {
         Serial.println("Couldn't load plugin!");
         resetFunc();    
    }
    delay(1000);
    //doTask();
    setupWatchDogTimer();
}

void loop()
{
      /*if(! isRecording)
      {
          enterSleep();
      }*/
      recordingFunc();
      //doTask();
}

void recordingFunc(void)
{
    if (!isRecording && !aboutToChange)
    {
        Serial.println("Begin recording");
        isRecording = true;
        
        // Check if the file exists already
//        char filename[15];
//        strcpy(filename, "RECORD00.txt");
//        for (uint8_t i = 0; i < 100; i++)
//        {
//            filename[6] = '0' + i/10;
//            filename[7] = '0' + i%10;
//            // create if does not exist, do not open existing, write, sync after write
//            if (! SD.exists(filename))
//            {
//                break;
//            }
//        }

        strcpy(RECORDING_NAME, "RECORD00.txt");
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

        if (! recording.open(RECORDING_NAME, O_CREAT | O_WRITE | O_SYNC | O_AT_END))
        {
             Serial.println("Couldn't open file to record!");
             resetFunc();
        }
        
        musicPlayer.startRecordOgg(true); // use microphone (for linein, pass in 'false')
    }
    if (isRecording)
    {
        saveRecordedData(isRecording);
    }
    if (isRecording && aboutToChange)
    {
        Serial.println("End recording");
        musicPlayer.stopRecordOgg();
        isRecording = false;
        //aboutToChange = false;
        saveRecordedData(isRecording);
        
        recording.close();

        while (SD.card()->isBusy())
        {
            Serial.println("SD BUSY");
        }
        Serial.println("DONE");
        //delay(1000);
        
        //writeDoneTask(1);
        //delay(25);
    }
}
//
///*void doTask()
//{
//    //setNextTask();
//    recordingFunc();
//}*/
//
uint16_t saveRecordedData(boolean isrecord)
{
    uint16_t written = 0;
    // read how many words are waiting for us
    uint16_t wordswaiting = musicPlayer.recordedWordsWaiting();    
    // try to process 256 words (512 bytes) at a time, for best speed
    while (wordswaiting > MWORDS)
    {
        Serial.print("Waiting: "); Serial.println(wordswaiting);
        // for example 128 bytes x 4 loops = 512 bytes
        for (int x = 0; x < MBYTES/RECBUFFSIZE; x++)
        {
        // fill the buffer!
            for (uint16_t addr = 0; addr < RECBUFFSIZE; addr+=2)
            {
                uint16_t t = musicPlayer.recordedReadWord();
                //Serial.println(t, HEX);
                recording_buffer[addr] = t >> 8; 
                recording_buffer[addr+1] = t;
            }
            if (! recording.write(recording_buffer, RECBUFFSIZE))
            {
                Serial.print("Couldn't write "); Serial.println(RECBUFFSIZE); 
                resetFunc();
            }
        }
        // flush 512 bytes at a time
        recording.flush();
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
            recording_buffer[addr] = t >> 8; 
            recording_buffer[addr+1] = t;
            if (addr > RECBUFFSIZE)
            {
                if (! recording.write(recording_buffer, RECBUFFSIZE))
                {
                    Serial.println("Couldn't write!");
                    resetFunc();
                }
                recording.flush();
                addr = 0;
            }
        }
        if (addr != 0)
        {
            if (!recording.write(recording_buffer, addr))
            {
                Serial.println("Couldn't write!");
                resetFunc();
            }
            recording.flush();
            written += addr;
        }
        musicPlayer.sciRead(VS1053_SCI_AICTRL3);
        
        if (! (musicPlayer.sciRead(VS1053_SCI_AICTRL3) & _BV(2)))
        {
            recording.write(musicPlayer.recordedReadWord() & 0xFF);
            written++;
        }
        recording.flush();
    }
    return written;
}

/*
void enterSleep(void)
{
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    
    sleep_mode();
    sleep_disable();
    
    power_all_enable();
    delay(25);
}*/

ISR(WDT_vect)
{
    aboutToChange = true;
}

void setupWatchDogTimer()
{
    
    MCUSR &= ~(1<<WDRF);
    WDTCSR |= (1<<WDCE) | (1<<WDE);
    WDTCSR  = (1 << WDP3) | (0 << WDP2) | (0 << WDP1) | (1 << WDP0);
    WDTCSR |= _BV(WDIE);
}

/*
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
}*/
