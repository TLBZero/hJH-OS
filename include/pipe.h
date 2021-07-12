/*
 * @Author: Yinwhe
 * @Date: 2021-07-10 20:06:58
 * @LastEditors: Yinwhe
 * @LastEditTime: 2021-07-12 14:02:16
 * @Description: file information
 * @Copyright: Copyright (c) 2021
 */
#ifndef __PIPE_H
#define __PIPE_H

#include "types.h"
#include "spinlock.h"
struct file;

#define PIPESIZE 512
#define SZ 4096

struct pipe{
    struct spinlock lock;
    char data[PIPESIZE];
    uint nread;
    uint nwrite;
    int readopen;
    int writeopen;
};

int pipealloc(int *sfd0, int *sfd1);
int pipeclose(struct file *f);
int pipewrite(struct file *f, char *str, int n);
int piperead(struct file *f, char *str, int n);
//void pipe_test();

#endif
