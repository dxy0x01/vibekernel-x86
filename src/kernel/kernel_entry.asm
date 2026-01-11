[bits 32]
[extern main] ; Define calling point. Must have same name as kernel.c 'main' function
global _start ; Linker needs this to be known

_start:
    call main ; Call the C function
    jmp $
