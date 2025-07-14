// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "kalloc.h"
#include "kernel/fdt.h"
#include "kernel/intr.h"
#include "memlayout.h"
#include "printf.h"
#include "riscv.h"
#include "spinlock.h"
#include "string.h"
#include "types.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.

uint64 pages = 0;
uint64 free_pages = 0;
// defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void kinit(void) {
  uint64 phystop = (PHYSTART + mem_size);
  initlock(&kmem.lock, "kmem");
  freerange(end, (void *)phystop);
  free_pages = pages;
  printf("kinit: %lu pages free (%lu bytes, %lu MB)\n", pages, pages * PGSIZE,
         (pages * PGSIZE) / (1024 * 1024));
  // Probe the beginning and end of each page to ensure they are accessible.
  char *p = NULL;
  for (p = end; p < (char *)phystop; p += PGSIZE) {
    *p = 0;                // write to the first byte of each page
  }
}

void freerange(void *pa_start, void *pa_end) {
  char *p = NULL;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE) {
    kfree(p);
    pages++;
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa) {
  struct run *r = NULL;
  uint64 phystop = PHYSTART + mem_size;
  if (pa == NULL || ((uint64)pa % PGSIZE) != 0 || (char *)pa < end ||
      (uint64)pa >= phystop) {
    panic("kfree");
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  free_pages++;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *kalloc(void) {
  struct run *r = NULL;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r) {
    kmem.freelist = r->next;
  }
  if (free_pages > 0) {
    free_pages--;
  }
  release(&kmem.lock);

  if (r) {
    memset((char *)r, 5, PGSIZE); // fill with junk
  }
  return (void *)r;
}

unsigned long get_free_pages(void) {
  unsigned long free = 0;

  acquire(&kmem.lock);
  free = free_pages;
  release(&kmem.lock);

  return free > 0 ? free : 0;
}

unsigned long get_total_pages(void) {
  unsigned long total = 0;

  acquire(&kmem.lock);
  total = pages;
  release(&kmem.lock);

  return total;
}