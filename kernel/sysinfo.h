#ifndef SYSINFO_H
#define SYSINFO_H

// Shared kernel-user memory page.
#include "kernel/types.h"
struct sysinfo {
  int pid;
  uint64 ticks;    // number of ticks since startup
  uint64 my_ticks; // number of ticks used by this process
  uint64 freemem;  // amount of free memory (bytes)
  uint64 nproc;    // number of processes
};

#endif