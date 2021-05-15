#include "sleeplock.h"
#include "spinlock.h"
#include "sched.h"
#include "put.h"

void acquiresleep(struct sleeplock *lk)
{
    acquire(&lk->lk);
    while (lk->lock)
    {
        sleep(lk, &lk->lk);
    }
    lk->lock = 1;
    lk->owner = getpid();
    #ifdef DEBUG
	printf("acqire sucessfully\n");
    #endif
    release(&lk->lk);
}

void releasesleep(struct sleeplock *lk)
{
    acquire(&lk->lk);
    lk->lock = 0;
    lk->owner = 0;
    wakeup(lk);
    #ifdef DEBUG
	printf("release sucessfully\n");
    #endif
    release(&lk->lk);
}

int holdingsleep(struct sleeplock *lk)
{
    int r;
    acquire(&lk->lk);
    r = lk->lock && (lk->owner == getpid());
    release(&lk->lk);
    return r;
}

void initsleeplock(struct sleeplock *lk, char *name)
{
    initlock(&(lk->lk), "sleep lock");
    lk->lock=0;
    lk->name=name;
    lk->owner=-1;
}
