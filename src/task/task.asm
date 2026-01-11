[BITS 32]

section .asm

global task_return

; void task_return(struct registers* regs)
task_return:
    mov ebp, esp
    ; Target registers structure
    mov ebx, [ebp+4]

    ; Push the data segment (SS)
    push dword [ebx+44]
    ; Push the stack pointer (ESP)
    push dword [ebx+40]
    ; Push the flags (EFLAGS)
    push dword [ebx+36]
    ; Push the code segment (CS)
    push dword [ebx+32]
    ; Push the instruction pointer (EIP)
    push dword [ebx+28]

    ; Setup Segment Registers
    mov ax, [ebx+44]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Pop general purpose registers
    mov edi, [ebx+0]
    mov esi, [ebx+4]
    mov ebp, [ebx+8]
    ; Skip ebx for now
    mov edx, [ebx+16]
    mov ecx, [ebx+20]
    mov eax, [ebx+24]
    
    ; Setup EBX last
    mov ebx, [ebx+12]

    ; Jump to User Mode
    iret
