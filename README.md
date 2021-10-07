# Invisible Probe: Timing Attacks with PCIe Congestion Side-channel

## Introduction
Our paper is published on [Oakland 21 S&P](https://www.computer.org/csdl/proceedings-article/sp/2021/893400b016/1t0x8TyhE8U) and the presentation slides are [here](InvisibleProbe.pdf).


## Probe
### RDMA Probe
Please make sure that you have connected two machines with Infiniband NIC and installed the drivers. The client and server can access each other.
```
client:
make
./rdma_probe <server_ip>

server:
make
./rdma_probe
```
The client will generate the log file.

### NVMe Probe
Firstly, you need to install SPDK and make sure you can run the "hello world" example. Please see the [offcial getting started](https://spdk.io/doc/getting_started.html).

Then put directory "nvme_probe" in the same directory "spdk". Type make and run the executable "nvmeProbe".( our probe is modified from the official "hello world" example.)
```
cd nvme_probe
make
sudo ./nvmeProbe
```

### Io_uring Probe
SPDK doesn't support devices like ESSD on Alibaba Cloud. So we use io_uring instead of it, which is supported since Linux 5.X.

## Collect probe traces
Use Google Chrome headless mode to access the web pages and collect the probe trace at the same time, and here is an easy demo.

## Show the traces
Interpret the logs and show the with matplotlib. The two easy scripts could show the data collected by rdma_probe and NVMe probe(io_uring probe is the same with nvme probe).
