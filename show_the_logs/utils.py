#! /usr/bin/python3
import os
import shutil
import struct
import numpy as np
import matplotlib.pyplot as plt

def read_records_rdma(filename="records.bin"):
    fin = open(filename, 'rb')
    fin.seek(0, os.SEEK_END)
    length = fin.tell()
    fin.seek(0, os.SEEK_SET)
    print("records length:", length)
    assert length % 0x18 == 0
    records_query = []
    records_rdtscp = []
    records_timestrap = []

    for i in range(length//0x18):
        r1 = fin.read(8)
        r2 = fin.read(8)
        r3 = fin.read(8)
        d1 = struct.unpack('1Q', r1)
        d2 = struct.unpack('1Q', r2)
        d3 = struct.unpack('1Q', r3)
        records_query.append(d1[0])
        records_rdtscp.append(d2[0])
        records_timestrap.append(d3[0])
    fin.close()
    return records_query, records_rdtscp, records_timestrap

def read_records_nvme(filename="records.bin"):
    fin = open(filename, 'rb')
    fin.seek(0, os.SEEK_END)
    length = fin.tell()
    fin.seek(0, os.SEEK_SET)
    print("records length:", length)
    assert length % 0x8 == 0
    records_rdtscp = []

    for i in range(length//0x8):
        r = fin.read(8)
        d = struct.unpack('1Q', r)
        records_rdtscp.append(d[0])
    fin.close()
    return records_rdtscp



def draw_picture(records_query=[], records_rdtscp=[], records_timestrap=[]):
    fig = plt.figure(figsize=(15, 8))

    ax = fig.add_subplot(3, 1, 1)
    ax.plot(range(len(records_query)), records_query, linewidth=0.15)
    plt.xlabel('timestamp id')
    plt.ylabel('query times')
    
    ax = fig.add_subplot(3, 1, 2)
    ARR = [(records_rdtscp[i] - records_rdtscp[i-1]) for i in range(1, len(records_rdtscp))]
    ax.plot(range(len(ARR)), ARR, linewidth=0.15)
    plt.xlabel('timestamp id')
    plt.ylabel('rdtscp(cycles)')

    ax = fig.add_subplot(3, 1, 3)
    ARR2 = [(records_timestrap[i] - records_timestrap[i-1]) for i in range(1, len(records_timestrap))]
    ax.plot(range(len(ARR2)), ARR2, linewidth=0.15)
    plt.xlabel('timestamp id')
    plt.ylabel('rdma cycles')
    plt.show()