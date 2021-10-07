#include <stdint.h>
static inline uint64_t rdtscp()
{
    uint64_t rax,rdx;
    asm volatile ( "rdtscp\n" : "=a" (rax), "=d" (rdx) : : "memory");
    return (rdx << 32) + rax;
}