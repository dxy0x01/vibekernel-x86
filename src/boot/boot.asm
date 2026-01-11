; VibeKernel-x86 Bootloader
; Simple "Hello World" bootloader that runs in 16-bit real mode

[BITS 16]           ; Tell NASM we're in 16-bit real mode
[ORG 0x7C00]
KERNEL_OFFSET equ 0x1000 ; Memory offset to which we will load our kernel

start:
    ; BOOTLOADER START
    mov [boot_drive], dl ; Save boot drive

    ; Initialize segment registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Print hello message
    mov si, msg_real_mode
    call print_string

    ; Load Kernel
    mov bx, KERNEL_OFFSET ; Read from disk and store in 0x1000
    mov dh, 15 ; Load 15 sectors (plenty of space)
    mov dl, [boot_drive]
    call disk_load

    ; Enable A20 Line
    call enable_a20

    ; Switch to Protected Mode
    call switch_to_pm

    jmp $

; Include helper files
%include "a20.asm"
%include "disk.asm"
%include "gdt.asm"
%include "print_string_pm.asm"
%include "switch_pm.asm"

[bits 32]
; This is where we arrive after switching to and initializing protected mode
BEGIN_PM:
    mov ebx, msg_prot_mode
    call print_string_pm    
    
    call KERNEL_OFFSET ; Jump to the loaded kernel code
    
    jmp $ ; Hang

; Function: print_string
; Prints a null-terminated string using BIOS
; Input: SI = pointer to string
print_string:
    pusha           ; Save all registers
    mov ah, 0x0E    ; BIOS teletype output function

.loop:
    lodsb           ; Load byte at DS:SI into AL, increment SI
    test al, al     ; Optimized check
    jz .done
    int 0x10        ; Call BIOS interrupt to print character
    jmp .loop

.done:
    popa            ; Restore all registers
    ret

; Data section
msg_real_mode: db '16-bit Real Mode', 0
msg_prot_mode: db '32-bit Protected Mode', 0
boot_drive: db 0

; Padding and boot signature
times 510-($-$$) db 0   ; Pad with zeros to byte 510
dw 0xAA55               ; Boot signature (must be at bytes 510-511)
