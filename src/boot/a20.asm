[bits 16]

; Function: enable_a20
; Enables the A20 line using BIOS interrupt or Fast A20
enable_a20:
    pusha

    ; Method 1: BIOS Int 0x15, AX=0x2401
    mov ax, 0x2401
    int 0x15
    jnc .done       ; If carry not set, it worked
    
    ; Method 2: Fast A20 (Port 0x92)
    in al, 0x92
    or al, 2
    out 0x92, al
    
    ; Wait a bit?
    
.done:
    popa
    ret
