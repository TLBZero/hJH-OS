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
#include "sleeplock.h"

#define BSIZE 512
#define BUFNR 50


struct buffer_head {
	uint b_blocknr;		/* block number */
	uint8 b_dev;		/* device (0 = free) */
	int disk;
	uint8 b_uptodate;
	uint8 b_dirt;		/* 0-clean,1-dirty */
	uint8 b_count;		/* users using this block */
	struct sleeplock b_lock;
	struct buffer_head * b_prev;
	struct buffer_head * b_next;
	uchar buf[BSIZE];			/* Here we bind buffer and buffer head together */
};

void binit(void);
struct buffer_head* bread(uint dev, uint blocknr);
void brelse(struct buffer_head* bh);
void bwrite(struct buffer_head* bh);