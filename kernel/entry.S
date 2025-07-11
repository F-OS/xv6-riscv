        # qemu -kernel loads the kernel at 0x80000000
        # and causes each hart (i.e. CPU) to jump there.
        # kernel.ld causes the following code to
        # be placed at 0x80000000.
.section .text
.global _entry
_entry:
        # a0 = hartid, a1 = pointer to fdt
        # set up a stack for C.
        # stack0 is declared in start.c,
        # with a 4096-byte stack per CPU.
        # sp = stack0 + (hartid * 4096)
        la sp, stack0
        li a2, 1024*4
        addi a3, a0, 1
        mul a2, a2, a3
        add sp, sp, a2
        # jump to start() in start.c
        call start
spin:
        j spin
