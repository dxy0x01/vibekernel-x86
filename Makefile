# Makefile for VibeKernel-x86

# Tools
ASM = nasm
CC = gcc
LD = ld
QEMU = qemu-system-i386

# Flags
CFLAGS = -m32 -ffreestanding -c -fno-pie
LDFLAGS = -m elf_i386 -T linker.ld --oformat binary

# Directories
SRC_DIR = src
BOOT_DIR = $(SRC_DIR)/boot
KERNEL_DIR = $(SRC_DIR)/kernel
DRIVERS_DIR = $(SRC_DIR)/drivers
CPU_DIR = $(SRC_DIR)/cpu
BIN_DIR = bin

# Files
BOOTLOADER = $(BOOT_DIR)/boot.asm
BOOTLOADER_BIN = $(BIN_DIR)/boot.bin
KERNEL_ENTRY = $(KERNEL_DIR)/kernel_entry.asm
KERNEL_ENTRY_OBJ = $(BIN_DIR)/kernel_entry.o
KERNEL_C = $(KERNEL_DIR)/kernel.c
KERNEL_OBJ = $(BIN_DIR)/kernel.o
SCREEN_C = $(DRIVERS_DIR)/screen.c
SCREEN_OBJ = $(BIN_DIR)/screen.o
PORTS_C = $(DRIVERS_DIR)/ports.c
PORTS_OBJ = $(BIN_DIR)/ports.o
IDT_C = $(CPU_DIR)/idt.c
IDT_OBJ = $(BIN_DIR)/idt.o
ISR_C = $(CPU_DIR)/isr.c
ISR_OBJ = $(BIN_DIR)/isr.o
INTERRUPT_ASM = $(CPU_DIR)/interrupt.asm
INTERRUPT_OBJ = $(BIN_DIR)/interrupt.o
KERNEL_BIN = $(BIN_DIR)/kernel.bin
OS_IMAGE = $(BIN_DIR)/os-image.bin

# Targets
.PHONY: all clean run debug

all: $(OS_IMAGE)

# Create bin directory if it doesn't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Assemble bootloader
$(BOOTLOADER_BIN): $(BOOTLOADER) | $(BIN_DIR)
	$(ASM) -f bin -I src/boot/ $(BOOTLOADER) -o $(BOOTLOADER_BIN)

# Assemble kernel entry
$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY) | $(BIN_DIR)
	$(ASM) -f elf $(KERNEL_ENTRY) -o $(KERNEL_ENTRY_OBJ)

# Assemble interrupt stubs
$(INTERRUPT_OBJ): $(INTERRUPT_ASM) | $(BIN_DIR)
	$(ASM) -f elf $(INTERRUPT_ASM) -o $(INTERRUPT_OBJ)

# Compile C kernel
$(KERNEL_OBJ): $(KERNEL_C) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(KERNEL_C) -o $(KERNEL_OBJ)

# Compile Screen Driver
$(SCREEN_OBJ): $(SCREEN_C) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(SCREEN_C) -o $(SCREEN_OBJ)

# Compile Ports Driver
$(PORTS_OBJ): $(PORTS_C) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(PORTS_C) -o $(PORTS_OBJ)

# Compile IDT
$(IDT_OBJ): $(IDT_C) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(IDT_C) -o $(IDT_OBJ)

# Compile ISR
$(ISR_OBJ): $(ISR_C) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(ISR_C) -o $(ISR_OBJ)

# Link kernel
$(KERNEL_BIN): $(KERNEL_ENTRY_OBJ) $(KERNEL_OBJ) $(SCREEN_OBJ) $(PORTS_OBJ) $(IDT_OBJ) $(ISR_OBJ) $(INTERRUPT_OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $(KERNEL_BIN) $(KERNEL_ENTRY_OBJ) $(KERNEL_OBJ) $(SCREEN_OBJ) $(PORTS_OBJ) $(IDT_OBJ) $(ISR_OBJ) $(INTERRUPT_OBJ)

# Create OS image (bootloader + kernel)
$(OS_IMAGE): $(BOOTLOADER_BIN) $(KERNEL_BIN)
	cat $(BOOTLOADER_BIN) $(KERNEL_BIN) > $(OS_IMAGE)
	# Pad with zeros to ensure we have enough sectors for the disk read
	truncate -s 32k $(OS_IMAGE)
	@echo "OS Image built successfully! Size: $$(wc -c < $(OS_IMAGE)) bytes"
	@ls -lh $(OS_IMAGE)

clean:
	rm -rf $(BIN_DIR)
	@echo "Cleaned build artifacts"

run: all
	$(QEMU) -drive format=raw,file=$(OS_IMAGE) -monitor stdio
