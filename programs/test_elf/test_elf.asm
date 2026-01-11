[BITS 32]

section .text
global _start

_start:
    ; Write 'E' (for ELF) to top-left of screen with bright white on blue background
    mov byte [0xB8000], 'E'
    mov byte [0xB8001], 0x1F

loop:
    jmp loop
