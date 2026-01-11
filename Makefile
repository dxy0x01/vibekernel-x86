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
OS_IMAGE = $(BIN_DIR)/os-image.bin
DATA_FILE = hei.txt

# Targets
.PHONY: all clean run debug

all: $(OS_IMAGE)

# Create bin directory if it doesn't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Assemble bootloader
$(BOOTLOADER_BIN): $(BOOTLOADER) | $(BIN_DIR)
	$(ASM) -f bin -I src/boot/ $(BOOTLOADER) -o $(BOOTLOADER_BIN)

# Create OS image (bootloader + data + padding)
$(OS_IMAGE): $(BOOTLOADER_BIN) $(DATA_FILE)
	cat $(BOOTLOADER_BIN) $(DATA_FILE) > $(OS_IMAGE)
	# Pad to ensure we have full sectors if needed, but for raw image it's fine.
	# Actually, to make it clean, let's pad hei.txt or just ensure qemu reads it.
	@echo "OS Image built successfully!"
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
