[bits 32]
; Define some constants
VIDEO_MEMORY equ 0xb8000
WHITE_ON_BLACK equ 0x0f

; prints a null-terminated string pointed to by EBX
print_string_pm:
    pusha
    mov edx, VIDEO_MEMORY ; Set edx to the start of video memory

.loop:
    mov al, [ebx]          ; Store the char at EBX in AL
    mov ah, WHITE_ON_BLACK ; Store the attributes in AH

    cmp al, 0              ; if (al == 0), at end of string, so
    je .done               ; jump to done

    mov [edx], ax          ; Store char and attributes at current character cell
    add ebx, 1             ; Increment EBX to the next char in string
    add edx, 2             ; Increment EDX to the next character cell in video memory

    jmp .loop              ; loop around to next char

.done:
    popa
    ret
