[BITS 32]

section .asm

global _start

_start:
    ; Write 'B' to top-left of screen with bright white on blue background
    mov byte [0xB8000], 'B'
    mov byte [0xB8001], 0x1F

loop:
    jmp loop
