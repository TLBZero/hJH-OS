#ifndef __TIMER_H
#define __TIMER_H

#include "types.h"
#include "spinlock.h"

#define INTERVAL 0x100000

extern struct spinlock tickslock;
extern uint ticks;

void timer_init();
void set_next_timeout();
void timer_tick();

#endif
