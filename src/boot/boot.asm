; VibeKernel-x86 Bootloader
; Simple "Hello World" bootloader that runs in 16-bit real mode

[BITS 16]           ; Tell NASM we're in 16-bit real mode
[ORG 0x7C00]        ; BIOS loads bootloader at address 0x7C00

start:
    ; Save boot drive number (passed in DL by BIOS)
    mov [boot_drive], dl

    ; Initialize segment registers
    xor ax, ax      ; Zero out AX
    mov ds, ax      ; Set Data Segment to 0
    mov es, ax      ; Set Extra Segment to 0
    mov ss, ax      ; Set Stack Segment to 0
    mov sp, 0x7C00  ; Set Stack Pointer (grows downward from bootloader)

    ; Clear screen
    call clear_screen

    ; Print hello message
    mov si, msg_real_mode
    call print_string

    ; Switch to Protected Mode
    call switch_to_pm

    jmp $

; Include helper files
%include "gdt.asm"
%include "print_string_pm.asm"
%include "switch_pm.asm"

[bits 32]
; This is where we arrive after switching to and initializing protected mode
BEGIN_PM:
    mov ebx, msg_prot_mode
    call print_string_pm    ; Use our 32-bit print function
    
    jmp $ ; Hang

; Function: clear_screen
; Clears the screen using BIOS interrupt
clear_screen:
    pusha           ; Save all registers
    mov ah, 0x00    ; Set video mode function
    mov al, 0x03    ; 80x25 text mode
    int 0x10        ; Call BIOS video interrupt
    popa            ; Restore all registers
    ret

; Function: print_string
; Prints a null-terminated string using BIOS
; Input: SI = pointer to string
print_string:
    pusha           ; Save all registers
    mov ah, 0x0E    ; BIOS teletype output function

.loop:
    lodsb           ; Load byte at DS:SI into AL, increment SI
    cmp al, 0       ; Check if null terminator
    je .done        ; If zero, we're done
    int 0x10        ; Call BIOS interrupt to print character
    jmp .loop       ; Continue loop

.done:
    popa            ; Restore all registers
    ret

; Data section
msg_real_mode: db 'Started in 16-bit Real Mode', 0x0D, 0x0A, 0
msg_prot_mode: db 'Successfully landed in 32-bit Protected Mode', 0
boot_drive: db 0

; Padding and boot signature
times 510-($-$$) db 0   ; Pad with zeros to byte 510
dw 0xAA55               ; Boot signature (must be at bytes 510-511)
