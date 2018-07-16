from time import sleep
from threading import Thread
from serial import Serial
from datetime import datetime

serial = Serial(port = "/dev/ttyUSB0", baudrate = 57600)

def keep():
    while True:
        a = serial.readline()
        try:
            a = a.decode().replace("\n", "")
            v = int(a)
            print(v)
            # binary(v)
        except:
            print(a)

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

# now = datetime.now()
#
# unix = int((now - datetime(1970,1,1)).total_seconds())
# array = toArray(unix)
# number = to32(array)

thread = Thread(target=keep)
thread.setDaemon(False)
thread.start()
#
# sleep(2)
# serial.write([0] + array)
#
# sleep(0.5)
# serial.write([1])
