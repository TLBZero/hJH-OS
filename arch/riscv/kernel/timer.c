/*
 * @Author: Yinwhe
 * @Date: 2021-07-10 20:06:58
 * @LastEditors: Yinwhe
 * @LastEditTime: 2021-07-13 10:38:16
 * @Description: file information
 * @Copyright: Copyright (c) 2021
 */
#include "types.h"
#include "riscv.h"
#include "rustsbi.h"
#include "spinlock.h"
#include "timer.h"
#include "put.h"
// #define DEBUG
struct spinlock tickslock;
uint ticks;

void timer_init() {
    initlock(&tickslock, "time");
    set_next_timeout();
    printf("[timer_init]timerinit done!\n");
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
