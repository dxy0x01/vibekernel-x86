[bits 32]
[extern main]
[global _start]
[extern _bss_start]
[extern _bss_end]

_start:
    ; Set up our own stack at a safe location (512KB)
    mov esp, 0x80000
    
    ; Ensure 16-byte alignment as required by GCC
    and esp, 0xFFFFFFF0

    ; Zero out the .bss section
    mov edi, _bss_start
    mov ecx, _bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb
    
    call main
    jmp $
