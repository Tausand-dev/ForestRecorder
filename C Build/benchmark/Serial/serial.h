class UART
{
  public:
    unsigned long int baudrate;
    UART(unsigned long int);
    void setUART(void) volatile;
    void sendChar(char) volatile;
    void print(const char *) volatile;
    void println(const char *) volatile;

    void write(uint8_t) volatile;
    void write(uint16_t) volatile;
    void write(uint32_t) volatile;
    void write(int) volatile;
    void write(long int) volatile;

    void flush(void) volatile;
    void toBuffer(void) volatile;
    unsigned char read(void) volatile;
    uint8_t available(void) volatile;

  private:
    volatile char buffer[10];
    volatile uint8_t last_buffer;
    unsigned char getChar(void) volatile;
};
