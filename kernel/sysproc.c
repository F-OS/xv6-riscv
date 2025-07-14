#include "kernel/kalloc.h"
#include "kernel/sysinfo.h"
#include "proc.h"
#include "spinlock.h"
#include "syscall.h"
#include "trap.h"
#include "types.h"

uint64 sys_exit(void) {
  int n = 0;
  argint(0, &n);
  exit(n);
  return 0; // not reached
}

uint64 sys_getpid(void) { return myproc()->pid; }

uint64 sys_fork(void) { return fork(); }

uint64 sys_wait(void) {
  uint64 p = 0;
  argaddr(0, &p);
  return wait(p);
}

uint64 sys_sbrk(void) {
  uint64 addr = 0;
  int n = 0;

  argint(0, &n);
  addr = myproc()->sz;
  if (growproc(n) < 0) {
    return -1;
  }
  return addr;
}

uint64 sys_sleep(void) {
  int n = 0;
  uint ticks0 = 0;

  argint(0, &n);
  if (n <= 0) {
    return 0; // don't sleep if n <= 0
  }
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < (uint)n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64 sys_kill(void) {
  int pid = 0;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64 sys_uptime(void) {
  uint xticks = 0;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64 sys_memstat(void) {
  uint64 p = 0;

  argaddr(0, &p);

  struct {
    unsigned long total_pages;  // Total number of pages in the system
    unsigned long free_pages;   // Number of free pages
    unsigned long used_pages;   // Number of used pages
    unsigned long total_memory; // Total memory in bytes
    unsigned long free_memory;  // Free memory in bytes
    unsigned long used_memory;  // Used memory in bytes
    unsigned long page_size;    // Size of each page in bytes
  } memstat;
  uint64 total_pages = get_total_pages();
  uint64 free_pages = get_free_pages();
  uint64 used_pages = total_pages - free_pages;
  memstat.total_pages = total_pages;
  memstat.free_pages = free_pages;
  memstat.used_pages = used_pages;
  memstat.total_memory = total_pages * PGSIZE;
  memstat.free_memory = free_pages * PGSIZE;
  memstat.used_memory = used_pages * PGSIZE;
  memstat.page_size = PGSIZE;
  if (either_copyout(1, p, &memstat, sizeof(memstat)) < 0) {
    return -1;
  }
  return 0;
}

// Already have shared memory updated in scheduler, just force an update and
// copy it to the user's given address.
uint64 sys_sysinfo(void) {
  uint64 p = 0;
  struct sysinfo info;

  argaddr(0, &p);
  struct proc *curproc = myproc();
  if (p == 0) {
    return -1; // invalid address
  }

  if (either_copyout(1, p, curproc->kshare, sizeof(struct sysinfo)) < 0) {
    return -1; // failed to copy out
  }
  return 0;
}
