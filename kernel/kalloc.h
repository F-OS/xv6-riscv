#ifndef KALLOC_H
#define KALLOC_H

void *kalloc(void);
void kfree(void *pa);
void kinit(void);
unsigned long get_free_pages(void);
unsigned long get_total_pages(void);

#endif // KALLOC_H
