# LumenOS Makefile - Official Limine configuration

CC = gcc
ASM = nasm
LD = ld
QEMU = qemu-system-i386

CFLAGS = -ffreestanding -fno-stack-protector -fno-pic -m32 \
         -Wall -Wextra -nostdlib -g -std=gnu99 -O0 \
         -Iinclude -Ilibs -Ikernel -Imodules -Iapps -Idrivers

ASMFLAGS = -f elf32 -g
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

KERNEL = kernel.bin
ISO = lumenos.iso
LIMINE_DIR = limine

# Source files
C_FILES = $(shell find . -name "*.c" -not -path "./$(LIMINE_DIR)/*")
ASM_FILES = boot/boot.asm $(shell find modules -name "*.asm" 2>/dev/null)
C_OBJS = $(C_FILES:.c=.o)
ASM_OBJS = $(ASM_FILES:.asm=.o)
OBJS = $(C_OBJS) $(ASM_OBJS)

.PHONY: all clean run kernel iso test install-limine

all: $(ISO)

# Build kernel
$(KERNEL): $(OBJS)
	@echo "🔨 Linking kernel..."
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo "✅ Kernel: $@ ($$(stat -c%s $@) bytes)"

# Create ISO according to official Limine instructions
$(ISO): $(KERNEL) check-limine-files
	@echo "📀 Creating ISO according to Limine docs..."
	@rm -rf iso_root
	@mkdir -p iso_root
	
	# According to docs: files can be in root, limine, boot, or boot/limine
	# We'll use boot/limine directory structure
	@mkdir -p iso_root/boot/limine iso_root/EFI/BOOT
	
	# Copy kernel to root (can be anywhere accessible via boot:// protocol)
	@cp $(KERNEL) iso_root/
	
	# Copy BIOS files to boot/limine (as per documentation)
	@cp $(LIMINE_DIR)/limine-bios.sys iso_root/boot/limine/
	@cp $(LIMINE_DIR)/limine-bios-cd.bin iso_root/
	
	# Copy UEFI files for hybrid ISO
	@cp $(LIMINE_DIR)/BOOTX64.EFI iso_root/EFI/BOOT/ 2>/dev/null || true
	@cp $(LIMINE_DIR)/limine-uefi-cd.bin iso_root/ 2>/dev/null || true
	
	# Copy config file - can be in root, limine, boot, or boot/limine
	@if [ -f "limine.cfg" ]; then \
		cp limine.cfg iso_root/; \
		cp limine.cfg iso_root/boot/; \
		cp limine.cfg iso_root/boot/limine/; \
		echo "📋 Config copied to all supported locations"; \
	else \
		echo "❌ limine.cfg not found in project root!"; \
		echo "Create limine.cfg with:"; \
		echo "  :LumenOS"; \
		echo "  PROTOCOL=multiboot1"; \
		echo "  KERNEL_PATH=boot:///kernel.bin"; \
		exit 1; \
	fi
	
	@echo "📁 ISO structure:"
	@find iso_root -type f | sort
	
	# Create hybrid ISO as per official instructions
	@echo "🛠️ Creating hybrid ISO..."
	@if [ -f "iso_root/limine-uefi-cd.bin" ] && [ -f "iso_root/EFI/BOOT/BOOTX64.EFI" ]; then \
		echo "Creating UEFI+BIOS hybrid ISO..."; \
		xorriso -as mkisofs -R -r -J \
			-b limine-bios-cd.bin \
			-no-emul-boot -boot-load-size 4 -boot-info-table \
			--efi-boot limine-uefi-cd.bin \
			-efi-boot-part --efi-boot-image --protective-msdos-label \
			iso_root -o $(ISO) 2>/dev/null; \
	else \
		echo "Creating BIOS-only ISO..."; \
		xorriso -as mkisofs \
			-b limine-bios-cd.bin \
			-no-emul-boot -boot-load-size 4 -boot-info-table \
			--protective-msdos-label \
			iso_root -o $(ISO) 2>/dev/null; \
	fi
	
	# Install Limine bootloader on ISO (REQUIRED step!)
	@if [ -f "$(LIMINE_DIR)/limine" ]; then \
		echo "🔧 Installing Limine bootloader..."; \
		chmod +x "$(LIMINE_DIR)/limine" 2>/dev/null; \
		"$(LIMINE_DIR)/limine" bios-install $(ISO) 2>&1 | grep -v "already" || true; \
		echo "✅ Limine installed on ISO"; \
	else \
		echo "⚠️  Limine executable not found, ISO may not be bootable"; \
		echo "Run: make install-limine"; \
	fi
	
	@rm -rf iso_root
	@echo "🎉 ISO created: $(ISO)"
	@echo "Run: make run"

# Check for required Limine files
check-limine-files:
	@echo "🔍 Checking Limine files..."
	@if [ ! -f "$(LIMINE_DIR)/limine-bios.sys" ]; then \
		echo "❌ Missing: $(LIMINE_DIR)/limine-bios.sys"; \
		exit 1; \
	fi
	@if [ ! -f "$(LIMINE_DIR)/limine-bios-cd.bin" ]; then \
		echo "❌ Missing: $(LIMINE_DIR)/limine-bios-cd.bin"; \
		exit 1; \
	fi
	@echo "✅ All required Limine files found"

# Install Limine from release zip
install-limine:
	@echo "⬇️ Downloading Limine..."
	@mkdir -p $(LIMINE_DIR)
	@wget -q https://github.com/limine-bootloader/limine/releases/latest/download/limine-bin.zip -O /tmp/limine.zip
	@unzip -qjo /tmp/limine.zip "limine-bios.sys" -d $(LIMINE_DIR)/
	@unzip -qjo /tmp/limine.zip "limine-bios-cd.bin" -d $(LIMINE_DIR)/
	@unzip -qjo /tmp/limine.zip "limine-uefi-cd.bin" -d $(LIMINE_DIR)/ 2>/dev/null || true
	@unzip -qjo /tmp/limine.zip "BOOTX64.EFI" -d $(LIMINE_DIR)/ 2>/dev/null || true
	@unzip -qjo /tmp/limine.zip "limine" -d $(LIMINE_DIR)/ 2>/dev/null || true
	@rm -f /tmp/limine.zip
	@chmod +x $(LIMINE_DIR)/limine 2>/dev/null || true
	@echo "✅ Limine installed to $(LIMINE_DIR)/"

# Create minimal limine.cfg
create-cfg:
	@echo "📝 Creating limine.cfg..."
	@echo "# LumenOS Configuration" > limine.cfg
	@echo ":LumenOS" >> limine.cfg
	@echo "PROTOCOL=multiboot1" >> limine.cfg
	@echo "KERNEL_PATH=boot:///kernel.bin" >> limine.cfg
	@echo "CMDLINE=console=ttyS0" >> limine.cfg
	@echo "✅ limine.cfg created"

# Compile C files
%.o: %.c
	@mkdir -p $(dir $@)
	@echo "  C     $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile ASM files
%.o: %.asm
	@mkdir -p $(dir $@)
	@echo "  ASM   $<"
	@$(ASM) $(ASMFLAGS) $< -o $@

# Run in QEMU
run: $(ISO)
	@echo "🚀 Starting QEMU..."
	$(QEMU) -cdrom $(ISO) -serial stdio -vga std -m 256M

# Run with UEFI (if available)
run-uefi: $(ISO)
	@echo "🚀 Starting QEMU with UEFI..."
	$(QEMU) -cdrom $(ISO) -serial stdio -vga std -m 256M -bios /usr/share/ovmf/OVMF.fd

# Test kernel directly
test: $(KERNEL)
	@echo "🧪 Testing kernel directly (no bootloader)..."
	$(QEMU) -kernel $(KERNEL) -serial stdio -vga std -m 256M

# Clean
clean:
	@echo "🧹 Cleaning..."
	@rm -f $(OBJS) $(KERNEL) $(ISO)
	@find . -name "*.o" -delete 2>/dev/null || true

# Show ISO contents
iso-info: $(ISO)
	@echo "📂 ISO contents:"
	@xorriso -indev $(ISO) -find / -type f 2>/dev/null || \
	echo "Use: xorriso -indev $(ISO) -find / -type f"

# Shortcuts
kernel: $(KERNEL)
iso: $(ISO)

help:
	@echo "LumenOS Build System with Limine"
	@echo "================================="
	@echo "First time setup:"
	@echo "  1. make install-limine    # Download Limine"
	@echo "  2. make create-cfg        # Create config file"
	@echo "  3. make all               # Build everything"
	@echo "  4. make run               # Run in QEMU"
	@echo ""
	@echo "Other commands:"
	@echo "  make test        # Test kernel directly"
	@echo "  make clean       # Clean build files"
	@echo "  make iso-info    # Show ISO contents"
	@echo "  make kernel      # Build kernel only"