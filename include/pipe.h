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
int pipeclose(int fd);
int pipewrite(int fd, char *str, int n);
int piperead(int fd, char *str, int n);
//void pipe_test();

#endif
