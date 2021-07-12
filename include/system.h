/*
 * @Author: Yinwhe
 * @Date: 2021-07-10 20:06:58
 * @LastEditors: Yinwhe
 * @LastEditTime: 2021-07-12 13:08:36
 * @Description: file information
 * @Copyright: Copyright (c) 2021
 */
#pragma once
#include "riscv.h"

#define PROCNR       50  // maximum number of processes
#define NCPU          2  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define BUFNR        (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       1000  // size of file system in blocks
#define MAXPATH      260   // maximum file path name

static inline void intr_on(){
    s_csr(sstatus, SSTATUS_SIE);
}

static inline void intr_off(){
    c_csr(sstatus, SSTATUS_SIE);
}

static inline int intr_get(){
    uint64 x;
    r_csr(sstatus, x);
    return ((x & SSTATUS_SIE)!=0);
}