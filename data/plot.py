import numpy as np
import matplotlib.pyplot as plt

data = np.genfromtxt("nano.dat", skip_header = 1)

plt.plot(data[:, 0], data[:, 1], "-o", label = "Power on")
plt.plot(data[:, 0], data[:, 2], "-o", label = "Power Down mode")

plt.legend()

plt.xlabel("Voltage (V)")
plt.ylabel("Current (mA)")

plt.savefig("ArduinoPower.pdf")
