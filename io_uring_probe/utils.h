#include <liburing.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <termio.h>
#include "sys/time.h"
#include <stdint.h>				// standard integer types, e.g., uint32_t
#include <signal.h>				// for signal handler
#include <string.h>				// strerror() function converts errno to a text string for printing
#include <fcntl.h>				// for open()
#include <errno.h>				// errno support
#include <assert.h>				// assert() function
#include <sys/mman.h>			// support for mmap() function
#include <linux/mman.h>			// required for 1GiB page support in mmap()
#include <math.h>				// for pow() function used in RAPL computations
#include <time.h>
#include <sys/time.h>			// for gettimeofday
#include <termio.h>
#include <sched.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <x86intrin.h>
#include <time.h>
#include <emmintrin.h>
#include <x86intrin.h>
#include <sys/stat.h>


static inline uint64_t rdtscp()
{
    uint64_t rax,rdx;
    asm volatile ( "rdtscp\n" : "=a" (rax), "=d" (rdx)::"%rcx","memory");
    return (rdx << 32) | rax;
}
