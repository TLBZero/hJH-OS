/**
 * @file buffer.h
 * @author hJH-Yin
 * @brief buffer for fs usage
 * @version 0.1
 * @date 2021-05-12
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#pragma once
#include "types.h"

#define BSIZE 512
#define BUFNR 


struct buffer_head {
	unsigned long b_blocknr;	/* block number */
	unsigned short b_dev;		/* device (0 = free) */
	unsigned char b_uptodate;
	unsigned char b_dirt;		/* 0-clean,1-dirty */
	unsigned char b_count;		/* users using this block */
	struct sleeplock b_lock;
	struct buffer_head * b_prev;
	struct buffer_head * b_next;
	uchar buf[BSIZE];			/* Here we bind buffer and buffer head together */
};

void binit(void);
struct buffer_head* bread(uint, uint);
void brelse(struct buffer_head*);
void bwrite(struct buffer_head*);