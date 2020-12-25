#define BAUDRATE 57600

#define SET_TIME_COMMAND 0x01
#define GET_TIME_COMMAND 0x02
#define RESET_COMMAND 0x03

// #define EXPAND_SIZE 1048576 //bytes of 1 minute recording

#define OK_ERROR 1
#define SD_ERROR 2

#define READ_SCHEDULE_ERROR 3
#define READ_PLUGIN_ERROR 4

#define RTC_ERROR 5
#define VS1053_ERROR 6
#define WRITE_ERROR 7

#define LED_PORT PORTC
#define LED_DDR DDRC
#define LED_PIN PC3
#define LED_DELAY 150

void sendTime(void);
void initSystems(void);
void serialHandler(void);
void reset(uint8_t error);
void ledFlicker(uint8_t n);

void activateINT0(void);
void deactivateINT0(void);

void writeReset(uint8_t code);

void makeRecordOgg(const char *name, uint16_t seconds);
void makeRecordWAV(const char *name, uint16_t sample_rate, uint16_t seconds);
