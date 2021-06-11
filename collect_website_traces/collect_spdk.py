#! /usr/bin/env python
import os
import shutil
import time
import psutil
import signal
import numpy as np
from multiprocessing import Process

def read_website_list(filename="website_list.txt"):
    l = open(filename).read().split()
    return l

NUM = 10
website_list = read_website_list("website_toy.txt")

def run_proc(cmd_name, CPU):
    p = psutil.Process()
    p.cpu_affinity([CPU])

    os.system(cmd_name)
    print('Run child process %s (%s)...' % (cmd_name, os.getpid()))

for j in range(0, NUM):
    print("round: "+str(j))
    site = website_list[j]
    
    time_line = Process(target=run_proc, args=('XXXXX/nvmeProbe', 1, 0))

    cmd = "google-chrome --headless --new-window --incognito --disable-application-cache --disable-gpu  --no-sandbox  %s" % (site)
    ac_net = Process(target=run_proc, args=(cmd, 1))

    name = site.split('/')[2]
    time_line.start()
    time.sleep(1)
    ac_net.start()
    ac_net.join()
    time_line.join()

    os.system("ps -ef | grep \"chrome\" |grep -v \'ii\' | cut -c 9-15  | xargs kill -9")
    os.system("cp XXXXXX/records.bin YYYYYY/spdk_"+name+str(j))