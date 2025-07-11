#include "param.h"
#include "riscv.h"
#include "types.h"

void kmain(uint64 hartid, uint64 fdt);
void timerinit(void);
#define SBI
// entry.S needs one stack per CPU.
char stack0[4096 * NCPU];

// entry.S jumps here in machine mode on stack0.
void start(uint64 hartid, uint64 fdt)
{
  #ifndef SBI
  // set M Previous Privilege mode to Supervisor, for mret.
  unsigned long x = r_mstatus();
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_S;
  w_mstatus(x);

  // set M Exception Program Counter to main, for mret.
  // requires gcc -mcmodel=medany
  w_mepc((uint64)kmain);

  // disable paging for now.
  w_satp(0);

  // delegate all interrupts and exceptions to supervisor mode.
  w_medeleg(0xffff);
  w_mideleg(0xffff);
  w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);

  // configure Physical Memory Protection to give supervisor mode
  // access to all of physical memory.
  w_pmpaddr0(0x3fffffffffffffULL);
  w_pmpcfg0(0xf);

  // ask for clock interrupts.
  timerinit();

  // keep each CPU's hartid in its tp register, for cpuid().
  int id = r_mhartid();
  w_tp(id);

  // switch to supervisor mode and jump to main().
  asm volatile("mret");
  #else
  // disable paging for now.
  w_satp(0);
  sfence_vma();

  // disable interrupts till sbi is initialized.
  w_sstatus(r_sstatus() & ~SSTATUS_SIE);
  // keep each CPU's hartid in its tp register.
  w_tp(hartid);

  kmain(hartid, fdt);

  #endif
}

// ask each hart to generate timer interrupts.
void timerinit(void) {
  // enable supervisor-mode timer interrupts.
  w_mie(r_mie() | MIE_STIE);

  // enable the sstc extension (i.e. stimecmp).
  w_menvcfg(r_menvcfg() | (1ULL << 63));

  // allow supervisor to use stimecmp and time.
  w_mcounteren(r_mcounteren() | 2);

  // ask for the very first timer interrupt.
  w_stimecmp(r_time() + 100000);
}
