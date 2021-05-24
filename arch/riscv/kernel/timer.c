#include "types.h"
#include "riscv.h"
#include "rustsbi.h"
#include "spinlock.h"
#include "timer.h"
#include "put.h"

struct spinlock tickslock;
uint ticks;

void timer_init() {
    initlock(&tickslock, "time");
    set_next_timeout();
    #ifdef DEBUG
    printf("[timer_init]timerinit donw!\n");
    #endif
}

void
set_next_timeout() {
    uint64 time;
    r_csr(time, time);
    set_timer(time + INTERVAL);
}

void timer_tick() {
    ticks++;
    set_next_timeout();
}
