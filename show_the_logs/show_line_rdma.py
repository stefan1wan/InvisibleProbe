#!/usr/bin/python3
from utils import *

filename = "records.bin"
qurey, rdtscp, rdma = read_records_rdma(filename)

draw_picture(qurey, rdtscp, rdma)