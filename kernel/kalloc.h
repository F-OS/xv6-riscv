#ifndef KALLOC_H
#define KALLOC_H
#include "types.h"

void *kalloc(void);
void kfree(void *pa);
void kinit(void);

#endif // KALLOC_H