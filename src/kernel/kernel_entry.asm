[bits 32]
[extern main]
[global _start]

_start:
    ; Set up our own stack at a safe location (512KB)
    mov esp, 0x80000
    
    ; Ensure 16-byte alignment as required by GCC
    and esp, 0xFFFFFFF0
    
    call main
    jmp $
