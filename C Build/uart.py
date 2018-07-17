from time import sleep
from threading import Thread
from serial import Serial
from datetime import datetime

class RecorderSerial(Serial):
    def __init__(self, port, timeout = 2):
        super(RecorderSerial, self).__init__(port, baudrate = 57600, timeout = timeout)
        if not self.testRecorder():
            raise(Exception("Could not verify recorder."))

    def testRecorder(self):
        line = self.readline()
        try:
            line = self.decode(line)
            if line == "Connection":
                return True
        except:
            pass
        return False

    def decode(self, line):
        return line.decode().replace("\n", "")

    def setTime(self, time):
        array = toArray(time)
        message = [0x00] + array

        ans = ""

        while True:
            self.write(message)
            ans = serial.readline()
            try:
                ans = int(self.decode(ans))
                if ans == time:
                    break
            except:
                pass

    def getTime(self):
        self.write([1])
        ans = self.readline()
        ans = self.decode(ans)
        try:
            return int(ans)
        except:
            return ans

    def reset(self):
        self.write([2])

serial = RecorderSerial(port = "/dev/ttyUSB0")

def binary(v):
    print("{0:b}".format(v))

def toArray(val):
    mask = 0xff
    bytes = []
    for i in range(4):
        val = unix & (mask << (i * 8))
        val = val >> (i * 8)
        bytes.append(val)
    return bytes

def to32(bytes):
    return bytes[0] | bytes[1] << 8 | bytes[2] << 16 | bytes[3] << 24

print(serial.readline())

sleep(1)
print(serial.readline())

now = datetime.now()
unix = int((now - datetime(1970,1,1)).total_seconds())

serial.setTime(unix)
