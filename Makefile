CC = gcc
ASM = nasm
LD = ld
QEMU = qemu-system-i386

CFLAGS = -ffreestanding -fno-stack-protector -fno-pic -m32 \
         -Wall -Wextra -nostdlib -g -std=gnu99 -O0 \
         -Iinclude -Ilibs -Ikernel -Imodules -Iapps -Idrivers \
		 -w

ASMFLAGS = -f elf32 -g
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

KERNEL = kernel.bin
ISO = lumenos.iso
LIMINE_DIR = limine

C_FILES = $(shell find . -name "*.c" -not -path "./$(LIMINE_DIR)/*")
ASM_FILES = boot/boot.asm $(shell find modules -name "*.asm" 2>/dev/null)
C_OBJS = $(C_FILES:.c=.o)
ASM_OBJS = $(ASM_FILES:.asm=.o)
OBJS = $(C_OBJS) $(ASM_OBJS)

.PHONY: all clean run test install-limine create-cfg

all: $(ISO)

$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(ISO): $(KERNEL)
	@rm -rf iso_root
	@mkdir -p iso_root/boot/limine iso_root/EFI/BOOT
	@cp $(KERNEL) iso_root/
	@cp $(LIMINE_DIR)/limine-bios.sys iso_root/boot/limine/
	@cp $(LIMINE_DIR)/limine-bios-cd.bin iso_root/
	
	@if [ -f "$(LIMINE_DIR)/BOOTX64.EFI" ]; then \
		cp $(LIMINE_DIR)/BOOTX64.EFI iso_root/EFI/BOOT/; \
		cp $(LIMINE_DIR)/limine-uefi-cd.bin iso_root/; \
	fi
	
	@if [ -f "limine.conf" ]; then \
		cp limine.conf iso_root/; \
	else \
		@echo "ERROR: limine.conf not found"; exit 1; \
	fi
	
	@xorriso -as mkisofs \
		-b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--protective-msdos-label \
		iso_root -o $(ISO) 2>/dev/null
	
	@if [ -f "$(LIMINE_DIR)/limine" ]; then \
		chmod +x "$(LIMINE_DIR)/limine"; \
		"$(LIMINE_DIR)/limine" bios-install $(ISO) 2>&1 | grep -v "already" || true; \
	fi
	
	@rm -rf iso_root

install-limine:
	@mkdir -p $(LIMINE_DIR)
	@wget -q https://github.com/limine-bootloader/limine/releases/latest/download/limine-bin.zip -O /tmp/limine.zip
	@unzip -qjo /tmp/limine.zip "limine-bios.sys" "limine-bios-cd.bin" "limine" -d $(LIMINE_DIR)/ 2>/dev/null || true
	@rm -f /tmp/limine.zip

create-cfg:
	@echo ":LumenOS" > limine.conf
	@echo "PROTOCOL=multiboot1" >> limine.conf
	@echo "KERNEL_PATH=boot:///kernel.bin" >> limine.conf

%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	@mkdir -p $(dir $@)
	@$(ASM) $(ASMFLAGS) $< -o $@

run: $(ISO)
	$(QEMU) -cdrom $(ISO) -serial stdio -vga std -m 256M

test: $(KERNEL)
	$(QEMU) -kernel $(KERNEL) -serial stdio -vga std -m 256M

clean:
	@rm -f $(OBJS) $(KERNEL) $(ISO)
	@find . -name "*.o" -delete 2>/dev/null || true

kernel: $(KERNEL)
iso: $(ISO)