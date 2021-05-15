#include "spinlock.h"

#ifndef __SLEEPLOCK_H
#define __SLEEPLOCK_H

struct spinlock;

struct sleeplock {
  long lock;       // Is the lock held?
  struct spinlock lk; // spinlock protecting this sleep lock
  
  // For debugging:
  char *name;        // Name of lock.
  long owner;           // Process holding lock
};

void acquiresleep(struct sleeplock*);
void releasesleep(struct sleeplock*);
int holdingsleep(struct sleeplock*);
void initsleeplock(struct sleeplock*, char*);

#endif
