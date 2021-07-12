/*
 * @Author: Yinwhe
 * @Date: 2021-07-10 20:06:58
 * @LastEditors: Yinwhe
 * @LastEditTime: 2021-07-12 12:28:34
 * @Description: file information
 * @Copyright: Copyright (c) 2021
 */
#ifndef _BUDDY_H
#define _BUDDY_H

#include "vm.h"

#define PAGENUM_ROUNDUP(size)   (PAGE_ROUNDUP(size) >> PAGE_SHIFT)
#define PAGENUM_ROUNDDOWN(size) (PAGE_ROUNDDOWN(size) >> PAGE_SHIFT)

#define LEFT_CHILD(index)   ((2 * (index)) + 1)
#define RIGHT_CHILD(index)  ((2 * (index)) + 2)
#define PARENT(index)       ((index - 1) / 2)

#define IS_POW_OF_2(x) (!((x) & ((x)-1)))

#define BUDDY_BITMAP_SIZE (2*4096)

struct buddy
{
  unsigned long size;
  unsigned *bitmap;
};

void init_buddy_system(void);
void *alloc_pages(int);
void free_pages(void *);

#endif