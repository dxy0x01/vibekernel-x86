# VibeKernel-x86 Makefile
# Simple Makefile for building the bootloader

# Tools
ASM = nasm
QEMU = qemu-system-i386

# Directories
SRC_DIR = src
BOOT_DIR = $(SRC_DIR)/boot
BIN_DIR = bin

# Files
BOOTLOADER = $(BOOT_DIR)/boot.asm
BOOTLOADER_BIN = $(BIN_DIR)/boot.bin

# Targets
.PHONY: all clean run debug

all: $(BOOTLOADER_BIN)

# Create bin directory if it doesn't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Assemble bootloader
$(BOOTLOADER_BIN): $(BOOTLOADER) | $(BIN_DIR)
	$(ASM) -f bin $(BOOTLOADER) -o $(BOOTLOADER_BIN)
	@echo "Bootloader built successfully!"
	@ls -lh $(BOOTLOADER_BIN)

# Run in QEMU
run: $(BOOTLOADER_BIN)
	$(QEMU) -drive format=raw,file=$(BOOTLOADER_BIN) -monitor stdio

# Run in QEMU with debugging
debug: $(BOOTLOADER_BIN)
	$(QEMU) -drive format=raw,file=$(BOOTLOADER_BIN) -s -S &
	gdb -ex "target remote localhost:1234" \
	    -ex "set architecture i8086" \
	    -ex "break *0x7c00" \
	    -ex "continue"

# Clean build artifacts
clean:
	rm -rf $(BIN_DIR)
	@echo "Cleaned build artifacts"
