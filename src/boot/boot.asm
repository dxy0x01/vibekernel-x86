; VibeKernel-x86 Bootloader
; Simple "Hello World" bootloader that runs in 16-bit real mode

[BITS 16]           ; Tell NASM we're in 16-bit real mode
[ORG 0x7C00]        ; BIOS loads bootloader at address 0x7C00

start:
    ; Initialize segment registers
    xor ax, ax      ; Zero out AX
    mov ds, ax      ; Set Data Segment to 0
    mov es, ax      ; Set Extra Segment to 0
    mov ss, ax      ; Set Stack Segment to 0
    mov sp, 0x7C00  ; Set Stack Pointer (grows downward from bootloader)

    ; Clear screen
    call clear_screen

    ; Initialize serial port
    call init_serial

    ; Print hello message
    mov si, msg_hello
    call print_string
    
    ; --- IVT Code Start ---
    ; Hook interrupt 0x00 (Divide by Zero)
    cli
    xor ax, ax
    mov es, ax
    
    ; Store Offset (address of handler)
    mov word [es:0x00*4], handle_isr0
    
    ; Store Segment (0x0000)
    mov word [es:0x00*4+2], 0x0000
    
    sti
    
    ; Trigger Divide by Zero
    mov si, msg_trigger_zero
    call print_string
    
    xor ax, ax      ; Clear AX
    mov ax, 10      ; AX = 10
    mov bx, 0       ; BX = 0
    div bx          ; 10 / 0 -> Triggers Int 0
    
    ; Note: Control flow might not return here if the handler doesn't handle the return address pointer adjustment carefully,
    ; but for a simple test, we just want to see the print.
    ; Real mode 'div' fault pushes CS:IP pointing to the instruction that caused the fault, so 'iret' would retry it (infinite loop).
    ; We'll just halt in the handler or print and halt.

    ; --- IVT Code End ---

    ; Halt the system
    jmp $

; interrupt 0x44 handler
; interrupt 0x00 handler (Divide by Zero)
handle_isr0:
    pusha
    mov si, msg_zero_caught
    call print_string
    popa
    ; In real mode, divide error fault IP points to the instruction that caused it.
    ; If we iret, it will re-execute 'div' and fault again (infinite loop).
    ; We will just halt here to prove we caught it.
    mov si, msg_halting
    call print_string
    cli
    hlt

; Function: init_serial
; Initialize COM1 for serial output
init_serial:
    mov dx, 0x3f9       ; Interrupt Enable Register
    mov al, 0x00        ; Disable all interrupts
    out dx, al
    
    mov dx, 0x3fb       ; Line Control Register
    mov al, 0x80        ; Enable DLAB (set baud rate divisor)
    out dx, al
    
    mov dx, 0x3f8       ; Divisor Latch Low Byte
    mov al, 0x03        ; 38400 baud
    out dx, al
    
    mov dx, 0x3f9       ; Divisor Latch High Byte
    mov al, 0x00
    out dx, al
    
    mov dx, 0x3fb       ; Line Control Register
    mov al, 0x03        ; 8 bits, no parity, one stop bit
    out dx, al
    
    mov dx, 0x3fa       ; FIFO Control Register
    mov al, 0xC7        ; Enable FIFO, clear them, with 14-byte threshold
    out dx, al
    
    mov dx, 0x3f4       ; Modem Control Register
    mov al, 0x0B        ; IRQs enabled, RTS/DSR set
    out dx, al
    ret

; Function: print_serial
; Prints a string to COM1
print_serial:
    pusha
    mov dx, 0x3F8
.loop:
    lodsb
    cmp al, 0
    je .done
    out dx, al      ; Output character to serial port
    jmp .loop
.done:
    popa
    ret

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
; Prints a null-terminated string to BIOS and Serial
; Input: SI = pointer to string
print_string:
    pusha           ; Save all registers
    
    ; Print to serial first (preserves SI effectively because of pusha/popa in print_serial)
    call print_serial
    
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
msg_hello: db 'VibeKernel-x86 Bootloader', 0x0D, 0x0A
           db 'Hello World from Real Mode!', 0x0D, 0x0A, 0
msg_trigger_zero: db 'Triggering Divide by Zero...', 0x0D, 0x0A, 0
msg_zero_caught: db 'Divide by Zero Exception Caught!', 0x0D, 0x0A, 0
msg_halting: db 'Halting System.', 0x0D, 0x0A, 0

; Padding and boot signature
times 510-($-$$) db 0   ; Pad with zeros to byte 510
dw 0xAA55               ; Boot signature (must be at bytes 510-511)
