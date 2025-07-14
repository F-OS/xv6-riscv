    .global kernel_symbols
    .global kernel_symbols_size
    .section .rodata
kernel_symbols:
    .incbin "kernel/kernelsym.bin"
1:
kernel_symbols_size:
    .int 1b - kernel_symbols