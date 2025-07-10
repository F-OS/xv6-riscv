#include "intr.h"
#include "memlayout.h"
#include "plic.h"
#include "printf.h"
#include "proc.h"
#include "riscv.h"
#include "spinlock.h"
#include "syscall.h"
#include "trap.h"
#include "types.h"
#include "uart.h"
#include "virtio.h"

void lookup_interrupt(uint64 scause, uint64 sstatus, uint64 sepc,
                      bool *do_yield, bool is_kernel) {
  (void)sstatus;
  if (is_kernel) {
    // kernel trap handling
    if (scause == 0x8000000000000009ULL) {
      // PLIC
      int irq = plic_claim();
      if (irq == UART0_IRQ) {
        uartintr();
      } else if (irq == VIRTIO0_IRQ) {
        virtio_disk_intr();
      } else if (irq != 0) {
        printf("unexpected interrupt %d\n", irq);
      }
      if (irq) {
        plic_complete(irq);
      }
    } else if (scause == 0x8000000000000005ULL) {
      // timer interrupt
      struct cpu* c = mycpu();
      if (c->isboothart){
        acquire(&tickslock);
        ticks++;
        wakeup(&ticks);
        release(&tickslock);
      }
      w_stimecmp(r_time() + 100000);
      *do_yield = true;
    } else {
      printf("kerneltrap(): unexpected scause 0x%llx\n", scause);
    }
  } else {
    // user trap handling
    struct proc *p = myproc();
    // save user program counter.
    p->trapframe->epc = sepc;
    if (scause == 8) {
      // system call
      if (killed(p)) {
        exit(-1);
      }
      // sepc points to the ecall instruction,
      // but we want to return to the next instruction.
      p->trapframe->epc += 4;
      // an interrupt will change sepc, scause, and sstatus,
      // so enable only now that we're done with those registers.
      intr_on();
      syscall();
    } else if (scause == 0x8000000000000009ULL) {
      // PLIC
      int irq = plic_claim();
      if (irq == UART0_IRQ) {
        uartintr();
      } else if (irq == VIRTIO0_IRQ) {
        virtio_disk_intr();
      } else if (irq != 0) {
        printf("unexpected interrupt %d\n", irq);
      }
      if (irq) {
        plic_complete(irq);
      }
    } else if (scause == 0x8000000000000005ULL) {
      struct cpu* c = mycpu();
      if (c->isboothart){
        acquire(&tickslock);
        ticks++;
        wakeup(&ticks);
        release(&tickslock);
      }

      // ask for the next timer interrupt. this also clears
      // the interrupt request. 1000000 is about a tenth
      // of a second.
      w_stimecmp(r_time() + 100000);
      *do_yield = true;
    } else {
      printf("usertrap(): unexpected scause 0x%llx pid=%d\n", scause, p->pid);
      printf("            sepc=0x%llx stval=0x%llx\n", sepc, r_stval());
      setkilled(p);
    }
    if (killed(p)) {
      exit(-1);
    }
  }
}