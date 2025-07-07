#ifndef INTR_H
#define INTR_H

#include "types.h"

#define INT_USER 0
#define INT_KERNEL 1
void lookup_interrupt(uint64 scause, uint64 sstatus, uint64 sepc, int *do_yield,
                      int mode);

#endif // INTR_H