#ifndef __PIPE_H
#define __PIPE_H

#include "types.h"
#include "spinlock.h"
#include "sysfile.h"

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

int pipealloc(int *fd0, int *fd1);
int pipeclose(struct file *f);
int pipewrite(struct file *f, char *str, int n);
int piperead(struct file *f, char *str, int n);
//void pipe_test();

#endif
