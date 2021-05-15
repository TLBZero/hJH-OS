#ifndef __SPINLOCK_H
#define __SPINLOCK_H


struct spinlock
{
    long lock; // lock状态，0：未占用；1：占用
    char *name;
    long owner;
};

// 初始化 
void initlock(struct spinlock*, char*);

// 获得锁
void acquire(struct spinlock*);

// 释放锁
void release(struct spinlock*);

// 检查锁是否被占用，必须关闭中断
int holding(struct spinlock*);

#endif


