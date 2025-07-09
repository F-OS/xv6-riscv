#ifndef KERNEL_SPINLOCK_H
#define KERNEL_SPINLOCK_H
#include "types.h"
struct cpu;

// Mutual exclusion lock.
struct spinlock {
  bool locked; // Is the lock held?

  // For debugging:
  const char *name; // Name of lock.
  struct cpu *cpu;  // The cpu holding the lock.
};

void acquire(struct spinlock *lk);
int holding(struct spinlock *lk);
void initlock(struct spinlock *lk, const char *name);
void release(struct spinlock *lk);
void push_off(void);
void pop_off(void);

#endif // KERNEL_SPINLOCK_H
