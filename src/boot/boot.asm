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

    ; Initialize serial port
    call init_serial

    ; Print hello message
    mov si, msg_hello
    call print_string
    
    ; --- Disk Read Code Start ---
    mov si, msg_reading_disk
    call print_string

    ; Read 1 sector from disk
    mov ah, 0x02        ; BIOS read sector function
    mov al, 1           ; Read 1 sector
    mov ch, 0           ; Cylinder 0
    mov dh, 0           ; Head 0
    mov cl, 2           ; Sector 2 (1-based, 1 is bootloader)
    mov dl, [boot_drive] ; Drive number
    mov bx, 0x9000      ; Buffer address (ES:BX = 0:0x9000)
    int 0x13            ; Call BIOS disk interrupt
    
    jc disk_error       ; Jump if Carry Flag set (error)
    
    mov si, msg_disk_success
    call print_string
    
    ; Print loaded content
    mov si, 0x9000      ; Point SI to buffer
    call print_string   ; Print what we read
    
    jmp $

disk_error:
    mov si, msg_disk_error
    call print_string
    jmp $
    
    ; --- Disk Read Code End ---

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
msg_reading_disk: db 'Reading Sector 2 from Disk...', 0x0D, 0x0A, 0
msg_disk_success: db 'Disk Read Successful!', 0x0D, 0x0A, 'Content:', 0x0D, 0x0A, 0
msg_disk_error:   db 'Disk Read Failed!', 0x0D, 0x0A, 0
boot_drive: db 0

; Padding and boot signature
times 510-($-$$) db 0   ; Pad with zeros to byte 510
dw 0xAA55               ; Boot signature (must be at bytes 510-511)
