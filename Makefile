# Makefile for VibeKernel-x86

# Tools
ASM = nasm
CC = gcc
LD = ld
QEMU = qemu-system-i386

# Flags
CFLAGS = -m32 -ffreestanding -c -fno-pie -mno-sse -mno-mmx -mno-sse2
LDFLAGS = -m elf_i386 -T linker.ld --oformat binary

# Directories
SRC_DIR = src
BOOT_DIR = $(SRC_DIR)/boot
KERNEL_DIR = $(SRC_DIR)/kernel
DRIVERS_DIR = $(SRC_DIR)/drivers
CPU_DIR = $(SRC_DIR)/cpu
MEMORY_DIR = $(SRC_DIR)/memory
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
DISK_STREAM_C = $(DRIVERS_DIR)/disk_stream.c
DISK_STREAM_OBJ = $(BIN_DIR)/disk_stream.o
FAT16_C = $(SRC_DIR)/fs/fat16.c
FAT16_OBJ = $(BIN_DIR)/fat16.o
VFS_C = $(SRC_DIR)/fs/file.c
VFS_OBJ = $(BIN_DIR)/file.o
PANIC_C = $(KERNEL_DIR)/panic.c
PANIC_OBJ = $(BIN_DIR)/panic.o
TASK_C = $(SRC_DIR)/task/task.c
TASK_OBJ = $(BIN_DIR)/task.o
TASK_ASM = $(SRC_DIR)/task/task.asm
TASK_ASM_OBJ = $(BIN_DIR)/task_asm.o
PROCESS_C = $(SRC_DIR)/task/process.c
PROCESS_OBJ = $(BIN_DIR)/process.o
GDT_C = $(CPU_DIR)/gdt.c
GDT_OBJ = $(BIN_DIR)/gdt.o
GDT_ASM = $(CPU_DIR)/gdt.asm
GDT_ASM_OBJ = $(BIN_DIR)/gdt_asm.o
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

# Compile Heap
$(BIN_DIR)/kheap.o: $(MEMORY_DIR)/heap/kheap.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(MEMORY_DIR)/heap/kheap.c -o $(BIN_DIR)/kheap.o

# Compile Paging
$(BIN_DIR)/paging.o: $(MEMORY_DIR)/paging/paging.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(MEMORY_DIR)/paging/paging.c -o $(BIN_DIR)/paging.o

# Compile Ports Driver
$(PORTS_OBJ): $(PORTS_C) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(PORTS_C) -o $(PORTS_OBJ)

# Compile Serial Driver
$(BIN_DIR)/serial.o: $(DRIVERS_DIR)/serial.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(DRIVERS_DIR)/serial.c -o $(BIN_DIR)/serial.o

# Compile ATA Driver
$(BIN_DIR)/ata.o: $(DRIVERS_DIR)/ata.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(DRIVERS_DIR)/ata.c -o $(BIN_DIR)/ata.o

# Compile Disk Stream
$(DISK_STREAM_OBJ): $(DISK_STREAM_C) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(DISK_STREAM_C) -o $(DISK_STREAM_OBJ)

# Compile FAT16
$(FAT16_OBJ): $(FAT16_C) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(FAT16_C) -o $(FAT16_OBJ)

# Compile String Utility
$(BIN_DIR)/string.o: $(SRC_DIR)/string/string.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(SRC_DIR)/string/string.c -o $(BIN_DIR)/string.o

# Compile VFS
$(VFS_OBJ): $(VFS_C) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(VFS_C) -o $(VFS_OBJ)

# Compile Path Parser

# Compile Path Parser
$(BIN_DIR)/path_parser.o: $(SRC_DIR)/fs/path_parser.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(SRC_DIR)/fs/path_parser.c -o $(BIN_DIR)/path_parser.o

# Compile IDT
$(IDT_OBJ): $(IDT_C) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(IDT_C) -o $(IDT_OBJ)

# Compile ISR
$(ISR_OBJ): $(ISR_C) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(ISR_C) -o $(ISR_OBJ)

# Compile GDT
$(GDT_OBJ): $(GDT_C) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(GDT_C) -o $(GDT_OBJ)

# Compile Panic
$(PANIC_OBJ): $(PANIC_C) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(PANIC_C) -o $(PANIC_OBJ)

# Compile Task and Process
$(TASK_OBJ): $(TASK_C) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(TASK_C) -o $(TASK_OBJ)

$(TASK_ASM_OBJ): $(TASK_ASM) | $(BIN_DIR)
	$(ASM) -f elf $(TASK_ASM) -o $(TASK_ASM_OBJ)

$(PROCESS_OBJ): $(PROCESS_C) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(PROCESS_C) -o $(PROCESS_OBJ)

$(GDT_ASM_OBJ): $(GDT_ASM) | $(BIN_DIR)
	$(ASM) -f elf $(GDT_ASM) -o $(GDT_ASM_OBJ)

# Link kernel
$(KERNEL_BIN): $(KERNEL_ENTRY_OBJ) $(GDT_ASM_OBJ) $(GDT_OBJ) $(KERNEL_OBJ) $(SCREEN_OBJ) $(PORTS_OBJ) $(IDT_OBJ) $(ISR_OBJ) $(INTERRUPT_OBJ) $(BIN_DIR)/kheap.o $(BIN_DIR)/paging.o $(BIN_DIR)/serial.o $(BIN_DIR)/ata.o $(DISK_STREAM_OBJ) $(BIN_DIR)/string.o $(BIN_DIR)/path_parser.o $(FAT16_OBJ) $(VFS_OBJ) $(PANIC_OBJ) $(TASK_OBJ) $(TASK_ASM_OBJ) $(PROCESS_OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $(KERNEL_BIN) $(KERNEL_ENTRY_OBJ) $(GDT_ASM_OBJ) $(GDT_OBJ) $(KERNEL_OBJ) $(SCREEN_OBJ) $(PORTS_OBJ) $(IDT_OBJ) $(ISR_OBJ) $(INTERRUPT_OBJ) $(BIN_DIR)/kheap.o $(BIN_DIR)/paging.o $(BIN_DIR)/serial.o $(BIN_DIR)/ata.o $(DISK_STREAM_OBJ) $(BIN_DIR)/string.o $(BIN_DIR)/path_parser.o $(FAT16_OBJ) $(VFS_OBJ) $(PANIC_OBJ) $(TASK_OBJ) $(TASK_ASM_OBJ) $(PROCESS_OBJ)

# Create OS image (bootloader + kernel)
$(OS_IMAGE): $(BOOTLOADER_BIN) $(KERNEL_BIN)
	cat $(BOOTLOADER_BIN) $(KERNEL_BIN) > $(OS_IMAGE)
	# Pad with zeros to ensure we have enough sectors for the disk read
	truncate -s 1M $(OS_IMAGE)
	@echo "OS Image built successfully! Size: $$(wc -c < $(OS_IMAGE)) bytes"
	@ls -lh $(OS_IMAGE)

clean:
	rm -rf $(BIN_DIR)
	@echo "Cleaned build artifacts"

# Create fat16 test image
$(BIN_DIR)/fat16.img: | $(BIN_DIR)
	dd if=/dev/zero of=$(BIN_DIR)/fat16.img bs=1M count=16
	mkfs.fat -F 16 $(BIN_DIR)/fat16.img

run: all $(BIN_DIR)/fat16.img
	$(QEMU) -drive format=raw,file=$(OS_IMAGE) -drive format=raw,file=$(BIN_DIR)/fat16.img -serial mon:stdio
