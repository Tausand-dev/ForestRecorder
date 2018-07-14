class UART
{
  public:
    unsigned long int baudrate;
    char buffer[32];
    static uint8_t last_buffer;

    UART(unsigned long int baud);
    void setUART(void);
    void sendChar(char tosend);
    void print(const char *text);
    void println(const char *text);
    void toBuffer(void);
    char read(void);
    uint8_t available(void);

  private:
    char getChar(void);
};
