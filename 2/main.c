//*******************************************************************
//        **** MAIN routine for microSD Data-Logger ****
//*******************************************************************
//Controller		: ATmega32 (Clock: 8 Mhz-internal)
//Compiler			: AVR-GCC (winAVR with AVRStudio-4)
//Project Version	: DL_1.0
//Author			: CC Dharmani, Chennai (India)
//			  		  www.dharmanitech.com
//Date				: 10 May 2011
//*******************************************************************

//visit the webpage of the project at below link: 
//  http://www.dharmanitech.com/2011/05/microsd-atmega32-datalogger.html

#define F_CPU 8000000UL		//freq 8 MHz
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "SPI_routines.h"
#include "SD_routines.h"
#include "UART_routines.h"
#include "RTC_routines.h"
#include "i2c_routines.h"
#include "ADC_routines.h"
#include "FAT32.h"

#define INTERVAL 1000 	//interval in milliseconds, between two measurements
						//interval defined here is approximate only, as the overhead delays
						//are not added, error is more for smaller values (i.e. <100ms)
						//minimum: 10; maximum: 600000 (600 seconds)

#define KEY_PRESSED		(!(PINC & 0x80))
#define GREEN_LED_ON	PORTC |= 0x20
#define RED_LED_ON		PORTC |= 0x40
#define RED_LED_OFF		PORTC &= ~0x40


void port_init(void)
{
  PORTA = 0x00;
  DDRA  = 0x00;
  PORTB = 0x00; //pull-up on for pushbutton
  DDRB  = 0xBF; //MISO line i/p, rest o/p
  PORTC = 0x80; //pull-up for push-button
  DDRC  = 0x60; //PC7- i/p (pushbutton), PC6, PC5 - o/p (two LEDs)
  PORTD = 0x00;
  DDRD  = 0x00;
}


//call this routine to initialize all peripherals
void init_devices(void)
{
  cli();  //all interrupts disabled
  port_init();
  spi_init();
  twi_init();
  uart0_init();
  ADC_init();

  MCUCR = 0x00;
  GICR  = 0x00;
  TIMSK = 0x00; //timer interrupt sources
  //all peripherals are now initialized
}

//function to blink LED in case of any error
void blinkRedLED(void)
{
  while(1)  //blink red LED continuously, if error
  {
    RED_LED_ON;
	_delay_ms(400);
	RED_LED_OFF;
	_delay_ms(400);
  }

}

//*************************** MAIN *******************************//
int main(void)
{
  unsigned char option, error, i, j, data, channel;
  unsigned char fileName[13];
  unsigned int delay, k;

  _delay_ms(100);  //delay for VCC stabilization

  init_devices();

  GREEN_LED_ON;  //turn on green LED to indicate power on
  RED_LED_OFF; 	 //keep red LED off for now

  transmitString_F (PSTR("\n\r\n\r****************************************************"));
  transmitString_F (PSTR("\n\r         microSD Datalogger - by CC Dharmani "));
  transmitString_F (PSTR("\n\r****************************************************\n\r"));

  cardType = 0;

  for (i=0; i<10; i++)
  {
  	error = SD_init();
  	if(!error) break;
  }

  if(error)
  {
  	if(error == 1) transmitString_F(PSTR("SD card not detected.."));
  	if(error == 2) transmitString_F(PSTR("Card Initialization failed.."));
  
  	blinkRedLED();
  }

  switch (cardType)
  {
  	case 1:transmitString_F(PSTR("Standard Capacity Card (Ver 1.x) Detected!"));
  		   break;
  	case 2:transmitString_F(PSTR("High Capacity Card Detected!"));
  		   break;
  	case 3:transmitString_F(PSTR("Standard Capacity Card (Ver 2.x) Detected!"));
  		   break;
  	default:transmitString_F(PSTR("Unknown SD Card Detected!"));
  		   break; 
  }

  error = getBootSectorData (); //read boot sector and keep necessary data in global variables
  if(error) 
  {
   	transmitString_F (PSTR("\n\rFAT32 not found!"));  //FAT32 incompatible drive
   	blinkRedLED();
  }

  SPI_HIGH_SPEED;	//SCK - 4 MHz
  _delay_ms(1);   	//some delay for settling new spi speed

 //For displaying menu on hyper terminal, the key (psh-button) must be kept pressed while 
 //powering ON or while reset. If key is not kept pressed, the program will not display menu and it will 
 //simply wait for start recording command (i.e. pressing of key afterwards)

  if(KEY_PRESSED)
  while(1)
  {
  	transmitString_F(PSTR("\n\r\n\r> 0 : Exit the Menu"));
  	transmitString_F(PSTR("\n\r> 1 : Display current Date/Time"));
  	transmitString_F(PSTR("\n\r> 2 : Update Date"));
  	transmitString_F(PSTR("\n\r> 3 : Update Time"));
  	transmitString_F(PSTR("\n\r> 4 : Get file list"));
  	transmitString_F(PSTR("\n\r> 5 : Read File"));
  	transmitString_F(PSTR("\n\r> 6 : Delete File"));

  	transmitString_F(PSTR("\n\r\n\r> Enter the option:"));
  	option = receiveByte();
  	transmitByte(option);


	switch (option)
	{
		case '0':transmitString_F(PSTR("\n\rNormal operation started.."));
		  		 goto STOP;

		case '1':RTC_displayDate();
		  		 RTC_displayTime();
		  		 break;

		case '2':RTC_updateDate();
		  		 break;

		case '3':RTC_updateTime();
	      		 break;

		case '4':TX_NEWLINE;
  		  		 findFiles(GET_LIST,0);
          		 break;

		case '5':
		case '6':transmitString_F(PSTR("\n\rEnter file name: "));
          		 for(i=0; i<13; i++)
				 fileName[i] = 0x00;   //clearing any previously stored file name
          		 i=0;
          		 while(1)
          		 {
            		data = receiveByte();
            		if(data == 0x0d) break;  //'ENTER' key pressed
					if(data == 0x08)	//'Back Space' key pressed
	 				{ 
	   					if(i != 0)
	   					{ 
	     					transmitByte(data);
							transmitByte(' '); 
	     					transmitByte(data); 
	     					i--; 
	   					} 
	   					continue;     
	 				}
					if(data <0x20 || data > 0x7e) continue;  //check for valid English text character
					transmitByte(data);
            		fileName[i++] = data;
            		if(i==13){transmitString_F(PSTR(" file name too long..")); break;}
          		}
          		if(i>12) break;
       
	      		TX_NEWLINE;
		  		if(option == '5') 
				{ 
					error = readFile( READ, fileName);
					if(error == 1) transmitString_F(PSTR("File does not exist.."));
				}
		  		if(option == '6') deleteFile(fileName);
          		break;

    	default:transmitString_F(PSTR("\n\r\n\r Invalid option!\n\r"));
	}
  }

  
  while(1)
  { 
  	while(!KEY_PRESSED); 	//wait here for key-press, recording starts when key is pressed
  	_delay_ms(40);	   		//key debounce delay

  	if(!KEY_PRESSED) continue;

  	while(KEY_PRESSED);  	//wait here for key-depress
  	_delay_ms(40);	   		//key debounce delay

  
	RED_LED_ON;  //turn on red LED to indicate that recording has started

	while(1)
	{
		error = RTC_getDate();
		if(error) blinkRedLED();

		j=0;
		for(i=0; i<8; i++)
		{
			fileName[i] = date[j++];
			if(j==2 || j==5) j++;	//excluding the '/' character from date in the fileName
		}

		fileName[8] = '.';
		fileName[9] = 'C';
		fileName[10] = 'S';
		fileName[11] = 'V';

		error = RTC_getTime();
    	if(error) blinkRedLED();
								
		//From here onwards, gather data by appending strings in dataString
		//dataString is declared in FAT32.h
		//make sure dataString doesn't exceed its MAX_STRING_SIZE, defined in FAT32.h
		//Also, end the data string with '\r' & '\n' characters to maintain CSV format

		for(i=0; i<10; i++) dataString[i] = date[i];
		dataString[i++] = ',';

		for(j=0;j<8; j++)  dataString[i++] = time[j];
		dataString[i++] = ',';
		
		readTemperature(0); //read temperature from adc channel-0
		for(j=0;j<7; j++)  dataString[i++] = temperature[j];
		

		for(channel=1; channel<8; channel++)  //read voltages from ADC channel 1 to 7
		{
			dataString[i++] = ',';
			readVoltage(channel); 
			for(j=0;j<7; j++)  dataString[i++] = voltage[j];
		}

		dataString[i++] = '\r';
		dataString[i] = '\n';    //always end the string with these two characters,
								 //before calling the writeFile function

		error = writeFile(fileName);
		if(error) blinkRedLED();

		delay = INTERVAL / 10;
		for(k=0; k<delay; k++) 
		{
		 	_delay_ms(10);  //10ms delay in each loop
			
			if(KEY_PRESSED) 	//check for key-press
  			{
				_delay_ms(40);	   		//key debounce delay
  				if(KEY_PRESSED) 
  				{
					while(KEY_PRESSED);  	//wait here for key-depress
  					_delay_ms(40);	   		//key debounce delay
					goto STOP;
				}
			}
		}

	}//end of while(1)

	STOP: 
	RED_LED_OFF;  //recording stopped

  }//end of while(1)

  return 0;
		
}//end of main

  
//********** END *********** www.dharmanitech.com *************
