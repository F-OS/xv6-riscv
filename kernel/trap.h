#ifndef TRAP_H
#define TRAP_H
#include "types.h"
// trap.c
extern uint ticks;
void trapinit(void);
void trapinithart(void);
extern struct spinlock tickslock;
void usertrapret(void);

#endif // TRAP_H