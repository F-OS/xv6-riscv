#ifndef INTR_H
#define INTR_H

#include "types.h"
void lookup_interrupt(uint64 scause, uint64 sstatus, uint64 sepc, bool *do_yield,
                      bool is_kernel);

#endif // INTR_H