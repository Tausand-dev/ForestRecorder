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

// These are the pins used for the breakout example
#define BREAKOUT_RESET  9      // VS1053 reset pin (output)
#define BREAKOUT_CS     10     // VS1053 chip select pin (output)
#define BREAKOUT_DCS    8      // VS1053 Data/command select pin (output)
// These are common pins between breakout and shield
#define CARDCS 4     // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(BREAKOUT_RESET, BREAKOUT_CS, BREAKOUT_DCS, DREQ, CARDCS);

void setup() 
{
    Serial.begin(9600);
    Serial.println("Adafruit VS1053 Simple Test");

    if (! musicPlayer.begin())
    { // initialise the music player
       Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
       while (1);
    }
    Serial.println(F("VS1053 found"));
  
    if (!SD.begin(CARDCS))
    {
      Serial.println(F("SD failed, or not present"));
      while (1);  // don't do anything more
    }

    // list files
    printDirectory(SD.open("/"), 0);
    
    // Set volume for left, right channels. lower numbers == louder volume!
    musicPlayer.setVolume(20,20);
    musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
    
    //Serial.println(F("Playing track 001"));
    //musicPlayer.playFullFile("track001.mp3");
    // Play another file in the background, REQUIRES interrupts!
    Serial.println(F("Playing track 002"));
    musicPlayer.startPlayingFile("track002.mp3");
}

void loop()
{
    if (musicPlayer.stopped())
    {
        Serial.println("Done playing music");
        while (1)
        {
            delay(10);  // we're done! do nothing...
        }
    }
    if (Serial.available())
    {
        char c = Serial.read();
    
        // if we get an 's' on the serial console, stop!
        if (c == 's')
        {
            musicPlayer.stopPlaying();
        }
        
        // if we get an 'p' on the serial console, pause/unpause!
        if (c == 'p')
        {
            if (! musicPlayer.paused())
            {
                Serial.println("Paused");
                musicPlayer.pausePlaying(true);
            }
            else
            { 
                Serial.println("Resumed");
                musicPlayer.pausePlaying(false);
            }
        }
    }
    delay(100);
}

/// File listing helper
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