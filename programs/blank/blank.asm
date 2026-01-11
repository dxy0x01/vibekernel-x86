[BITS 32]
org 0x400000

section .asm

global _start

_start:
    ; Use syscall to print
    mov eax, 0 ; SYSCALL_PRINT
    mov ebx, msg
    int 0x80

loop:
    jmp loop

section .data
msg: db "Hello from User Mode via Syscall!", 0
