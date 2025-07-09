#include "bio.h"
#include "console.h"
#include "file.h"
#include "fs.h"
#include "kalloc.h"
#include "plic.h"
#include "printf.h"
#include "proc.h"
#include "trap.h"
#include "virtio.h"
#include "vm.h"

static volatile bool started = false;

// start() jumps here in supervisor mode on all CPUs.
void kmain(void) {
  if (cpuid() == 0) {
    consoleinit();
    printfinit();
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
    userinit();         // first user process
    __sync_synchronize();
    started = true;
  } else {
    while (started == false) {
      ;
    }
    __sync_synchronize();
    printf("hart %d starting\n", cpuid());
    kvminithart();  // turn on paging
    trapinithart(); // install kernel trap vector
    plicinithart(); // ask PLIC for device interrupts
  }

  scheduler();
}
