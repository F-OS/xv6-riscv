#ifndef VM_H
#define VM_H
#include "riscv.h"
#include "types.h"

void kvminit(void);
void kvminithart(void);
void kvmmap(pagetable_t kpgtbl, uint64 va, uint64 pa, uint64 sz, uint64 perm);
int mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa,
             int perm);
pagetable_t uvmcreate(void);
void uvmfirst(pagetable_t pagetable, uchar *src, uint sz);
uint64 uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz, int xperm);
uint64 uvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz);
int uvmcopy(pagetable_t old, pagetable_t new, uint64 sz);
void uvmfree(pagetable_t pagetable, uint64 sz);
void uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, bool do_free);
void uvmclear(pagetable_t pagetable, uint64 va);
pte_t *walk(pagetable_t pagetable, uint64 va, int alloc);
uint64 walkaddr(pagetable_t pagetable, uint64 va);
int copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len);
int copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len);
int copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len);

#endif // VM_H