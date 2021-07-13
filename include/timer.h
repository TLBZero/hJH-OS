/*
 * @Author: Yinwhe
 * @Date: 2021-07-10 20:06:58
 * @LastEditors: Yinwhe
 * @LastEditTime: 2021-07-13 10:40:54
 * @Description: file information
 * @Copyright: Copyright (c) 2021
 */
#ifndef __TIMER_H
#define __TIMER_H

#include "types.h"
#include "spinlock.h"

#define INTERVAL 0x10000

extern struct spinlock tickslock;
extern uint ticks;

void timer_init();
void set_next_timeout();
void timer_tick();

#endif
