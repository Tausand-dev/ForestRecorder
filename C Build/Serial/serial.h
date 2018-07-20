class UART
{
  public:
    unsigned long int baudrate;
    char buffer[10];

    UART(unsigned long int);
    void setUART(void);
    void sendChar(char);
    void print(const char *);
    void println(const char *);

    void write(uint8_t);
    void write(uint16_t);
    void write(uint32_t);
    void write(int);
    void write(long int);

    void flush(void);
    void toBuffer(void);
    unsigned char read(void);
    uint8_t available(void);

  private:
    static uint8_t last_buffer;
    unsigned char getChar(void);
};
