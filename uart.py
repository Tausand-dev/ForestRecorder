from time import sleep
from serial import Serial
from datetime import datetime

class RecorderSerial(Serial):
    def __init__(self, port, baudrate = 9600, timeout = 2):
        super(RecorderSerial, self).__init__(port, baudrate = baudrate, timeout = timeout)
        sleep(1)

    def decode(self, line):
        return line.decode().replace("\r\n", "")

    def setTime(self, time):
        temp = time.strftime("%d%m%y%H%M%S")

        ascii_time = [ord(i) for i in temp]
        message = [0x00] + ascii_time

        ans = ""

        while True:
            self.write(message)
            ans = self.decode(serial.readline())
            try:
                datetime.strptime(ans, '%d,%m,%y,%H,%M,%S')
                break
            except:
                pass

    def getTime(self):
        self.write([1])
        ans = self.readline()
        ans = self.decode(ans)
        try:
            return datetime.strptime(ans, '%d,%m,%y,%H,%M,%S')
        except:
            return ans

    def reset(self):
        self.write([2])

if __name__ == "__main__":
    serial = RecorderSerial(port = "COM5", timeout = 2)
    serial.readline()

    serial.setTime(datetime.now())
    current = serial.getTime()
    print(current)

    # reset arduino
    serial.reset()
    serial.close()
