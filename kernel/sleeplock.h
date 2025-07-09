#ifndef KERNEL_SLEEPLOCK_H
#define KERNEL_SLEEPLOCK_H

#include "spinlock.h"

// Long-term locks for processes
struct sleeplock {
  bool locked;        // Is the lock held?
  struct spinlock lk; // spinlock protecting this sleep lock

  // For debugging:
  const char *name; // Name of lock.
  int pid;          // Process holding lock
};

void acquiresleep(struct sleeplock *lk);
void releasesleep(struct sleeplock *lk);
int holdingsleep(struct sleeplock *lk);
void initsleeplock(struct sleeplock *lk, const char *name);

#endif // KERNEL_SLEEPLOCK_H
