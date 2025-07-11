#include "bio.h"
#include "console.h"
#include "file.h"
#include "fs.h"
#include "kalloc.h"
#include "kernel/sbi.h"
#include "plic.h"
#include "printf.h"
#include "proc.h"
#include "string.h"
#include "trap.h"
#include "virtio.h"
#include "vm.h"

// Highest HARTID allowed to start.
extern volatile int allowed_n = 0;
volatile static unsigned long main_hartid = ~0UL;
extern char _bss_start[], _bss_end[];

// start() jumps here in supervisor mode on all CPUs.
void kmain(int hartid, uint64 fdt) {
  if (main_hartid == ~0UL) {
    memset(_bss_start, 0, _bss_end - _bss_start);
    main_hartid = hartid;
    consoleinit();
    printfinit();
    sbiinit();
    putstr("\n");
    putstr("xv6 kernel is booting\n");
    putstr("\n");
    kinit();            // physical page allocator
    kvminit();          // create kernel page table
    kvminithart();      // turn on paging
    procinit();         // process table
    trapinit();         // trap vectors
    trapinithart();     // install kernel trap vector
    plicinit();         // set up interrupt controller
    plicinithart();     // ask PLIC for device interrupts
    binit();            // buffer cache
    iinit();            // inode table
    fileinit();         // file table
    virtio_disk_init(); // emulated hard disk
    timer_set();        // set up timer interrupts
    userinit();         // first user process
    printf("main hartid %d\n", main_hartid);
    __sync_synchronize();
    allowed_n += 1;
  } else {
    while(main_hartid == ~0UL) {
      __sync_synchronize(); // wait for main hartid to be set
    }
    int whoami = (hartid - main_hartid) % NCPU;
    while (allowed_n < whoami) {
      __sync_synchronize();
    }
    __sync_synchronize();
    printf("hart %d starting\n", whoami);
    kvminithart();  // turn on paging
    trapinithart(); // install kernel trap vector
    plicinithart(); // ask PLIC for device interrupts
    timer_set();
    __sync_synchronize();
    allowed_n += 1;
  }

  scheduler();
}
