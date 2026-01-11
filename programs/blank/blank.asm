[BITS 32]
org 0x400000

section .asm

global _start

global _start

_start:
    ; 1. Print start message
    mov eax, 0 ; SYSCALL_PRINT
    mov ebx, msg_start
    int 0x80

    ; 2. Open "1:/BLANK.BIN"
    mov eax, 1 ; SYSCALL_OPEN
    mov ebx, filename
    mov ecx, mode
    int 0x80
    
    ; Save FD or check for error
    cmp eax, 0
    jle error
    mov [fd], eax

    ; 3. Print success message
    mov eax, 0 ; SYSCALL_PRINT
    mov ebx, msg_open
    int 0x80

    ; 4. Stat the file
    mov eax, 4 ; SYSCALL_STAT
    mov ebx, [fd]
    mov ecx, stat_buf
    int 0x80

    cmp eax, 0
    jne error

    ; 5. Close the file
    mov eax, 3 ; SYSCALL_CLOSE
    mov ebx, [fd]
    int 0x80

    ; 6. Print end message
    mov eax, 0 ; SYSCALL_PRINT
    mov ebx, msg_done
    int 0x80

    ; 7. Exit
    mov eax, 5 ; SYSCALL_EXIT
    int 0x80

error:
    mov eax, 0 ; SYSCALL_PRINT
    mov ebx, msg_error
    int 0x80
    
    mov eax, 5 ; SYSCALL_EXIT
    int 0x80

section .data
filename: db "1:/BLANK.BIN", 0
mode: db "r", 0
msg_start: db "Starting Syscall Test...", 10, 0
msg_open: db "File opened successfully!", 10, 0
msg_done: db "Syscall test completed successfully.", 10, 0
msg_error: db "An error occurred during syscall test.", 10, 0

section .bss
fd: resd 1
stat_buf: resb 64 ; Plenty of space for struct file_stat
