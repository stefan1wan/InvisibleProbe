#!/usr/bin/python3

from utils import *

filename = "records.bin"

data = read_records_nvme(filename)

x1, y1 =[], []

for i in range(0, len(data)):
    x1.append(i)
    y1.append(data[i])

fig = plt.figure(1)
ax = fig.add_subplot(111)
ax.plot(x1,y1, linewidth=0.25)
plt.show()

