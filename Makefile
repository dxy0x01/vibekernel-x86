# VibeKernel-x86 Makefile
# Simple Makefile for building the bootloader

# Tools
ASM = nasm
CC = gcc
LD = ld
QEMU = qemu-system-i386

# Flags
CFLAGS = -m32 -ffreestanding -c -fno-pie
LDFLAGS = -m elf_i386 -Ttext 0x1000 --oformat binary

# Directories
SRC_DIR = src
BOOT_DIR = $(SRC_DIR)/boot
KERNEL_DIR = $(SRC_DIR)/kernel
BIN_DIR = bin

# Files
BOOTLOADER = $(BOOT_DIR)/boot.asm
BOOTLOADER_BIN = $(BIN_DIR)/boot.bin
KERNEL_ENTRY = $(KERNEL_DIR)/kernel_entry.asm
KERNEL_ENTRY_OBJ = $(BIN_DIR)/kernel_entry.o
KERNEL_C = $(KERNEL_DIR)/kernel.c
KERNEL_OBJ = $(BIN_DIR)/kernel.o
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

# Compile C kernel
$(KERNEL_OBJ): $(KERNEL_C) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(KERNEL_C) -o $(KERNEL_OBJ)

# Link kernel
$(KERNEL_BIN): $(KERNEL_ENTRY_OBJ) $(KERNEL_OBJ)
	$(LD) $(LDFLAGS) -o $(KERNEL_BIN) $(KERNEL_ENTRY_OBJ) $(KERNEL_OBJ)

# Create OS image (bootloader + kernel)
$(OS_IMAGE): $(BOOTLOADER_BIN) $(KERNEL_BIN)
	cat $(BOOTLOADER_BIN) $(KERNEL_BIN) > $(OS_IMAGE)
	# Pad with zeros to ensure we have enough sectors for the disk read
	truncate -s 10k $(OS_IMAGE)
	@echo "OS Image built successfully! Size: $$(wc -c < $(OS_IMAGE)) bytes"
	@ls -lh $(OS_IMAGE)

# Run in QEMU
run: $(OS_IMAGE)
	$(QEMU) -drive format=raw,file=$(OS_IMAGE) -monitor stdio

# Run in QEMU with debugging
debug: $(OS_IMAGE)
	$(QEMU) -drive format=raw,file=$(OS_IMAGE) -s -S &
	gdb -ex "target remote localhost:1234" \
	    -ex "set architecture i8086" \
	    -ex "break *0x7c00" \
	    -ex "continue"

# Clean build artifacts
clean:
	rm -rf $(BIN_DIR)
	@echo "Cleaned build artifacts"
