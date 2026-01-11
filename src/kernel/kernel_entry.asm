[bits 32]
[extern main] ; Define calling point. Must have same name as kernel.c 'main' function
global _start ; Linker needs this to be known

_start:
    ; Ensure stack is 16-byte aligned for GCC
    and esp, 0xFFFFFFF0
    call main ; Call the C function
    jmp $
