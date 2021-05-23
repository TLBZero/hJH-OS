#include "buffer.h"
#include "system.h"
#include "spinlock.h"
#include "put.h"
#include "sdcard.h"
#include "sleeplock.h"

struct buffer_head buf[BUFNR+1]; // buf[0] acts as head
static struct buffer_head* free_list;
static struct spinlock mutex;

/**
 * @brief Init buffer
 */
void binit(void){
    struct buffer_head *bh;
    free_list = &buf[0];
    initlock(&mutex,  "buffer");

    // Init buffer_head and link them together
    for(bh = free_list; bh < free_list+BUFNR; bh++){
        bh->b_blocknr=0;
        bh->b_count=0;
        bh->b_dev=-1;
        bh->b_dirt=0;
        initsleeplock(&bh->b_lock, "buffer");
        bh->b_uptodate = 0;
        bh->b_next = bh+1;
        bh->b_prev = bh-1;
    }
    bh--;
    free_list->b_prev = bh;
    bh->b_next = free_list;
    #ifdef DEBUG
    printf("[binit]Done!\n");
    #endif
}

static struct buffer_head* getblk(int dev, int block){
    struct buffer_head* bh;
    acquire(&mutex);
    // if in cache
    for(bh = free_list->b_next; bh!=free_list; bh=bh->b_next)
        if(bh->b_dev == dev && bh->b_blocknr == block){
            bh->b_count++;
            release(&mutex);
            acquiresleep(&bh->b_lock);
            return bh;
        }

    // Or new buffer shall be allocated (LRU Algorithm)
    for(bh = free_list->b_prev;bh!=free_list;bh=bh->b_prev)
        if(bh->b_count == 0){
            bh->b_count = 1;
            bh->b_dev = dev;
            bh->b_blocknr = block;
            bh->b_uptodate = 0;
            release(&mutex);
            acquiresleep(&bh->b_lock);
            return bh;
        }
    panic("getblk error, no free buffer found!");
}

static void inline liftBuffer(struct buffer_head *bh){
    acquire(&mutex);
    //Remove bh from original place
    bh->b_prev->b_next = bh->b_next;
    bh->b_next->b_prev = bh->b_prev;

    //Insert after head
    bh->b_next = free_list->b_next;
    bh->b_prev = free_list;
    free_list->b_next = bh;
    bh->b_next->b_prev = bh;
    release(&mutex);
}

static void inline downBuffer(struct buffer_head *bh){
    acquire(&mutex);
    //Remove bh from original place
    bh->b_prev->b_next = bh->b_next;
    bh->b_next->b_prev = bh->b_prev;

    //Insert before head
    bh->b_next = free_list;
    bh->b_prev = free_list->b_prev;
    free_list->b_prev = bh;
    bh->b_prev->b_next = bh;
    release(&mutex);
}

/**
 * @brief read buffer from disk or cache
 * 
 * @return return needed buffer, or NULL if fail
 */
struct buffer_head* bread(uint dev, uint blocknr){
    struct buffer_head *bh;
    bh = getblk(dev, blocknr);
    if(!bh->b_uptodate){
        sdcard_read_sector(bh->buf, blocknr);
        // Operation fails
        if(!bh->b_uptodate){
            brelse(bh);
            return NULL;
        }
    }
    liftBuffer(bh);
    return bh;
}

/**
 * @brief write buffer into disk
 */
void bwrite(struct buffer_head *bh){
    if(!holdingsleep(&bh->b_lock)) panic("[bwrite]Not holding lock!");
    sdcard_write_sector(bh->buf, bh->b_blocknr);
}

/**
 * @brief release buffer
 */
void brelse(struct buffer_head* bh){
    if(!bh) return; // NULL Pointer, no operations
    
    if(!(bh->b_count--)) panic("[brelse] Trying to free free buffer!\n");
    if(bh->b_count == 0) downBuffer(bh);
    releasesleep(&bh->b_lock);
}