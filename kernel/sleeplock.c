// Sleeping locks

#include "sleeplock.h"
#include "proc.h"
#include "spinlock.h"

void initsleeplock(struct sleeplock *lk, const char *name) {
  initlock(&lk->lk, "sleep lock");
  lk->name = name;
  lk->locked = false;
  lk->pid = 0;
}

void acquiresleep(struct sleeplock *lk) {
  acquire(&lk->lk);
  while (lk->locked) {
    sleep(lk, &lk->lk);
  }
  lk->locked = true;
  lk->pid = myproc()->pid;
  release(&lk->lk);
}

void releasesleep(struct sleeplock *lk) {
  acquire(&lk->lk);
  lk->locked = false;
  lk->pid = 0;
  wakeup(lk);
  release(&lk->lk);
}

int holdingsleep(struct sleeplock *lk) {
  int r;

  acquire(&lk->lk);
  r = lk->locked && (lk->pid == myproc()->pid);
  release(&lk->lk);
  return r;
}
