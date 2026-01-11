CC = gcc
ASM = nasm
LD = ld
QEMU = /mnt/c/Program\ Files/qemu/qemu-system-i386.exe

# Пути к основным директориям
SRC_DIR = src
BUILD_DIR = build

CFLAGS = -ffreestanding -fno-stack-protector -fno-pic -m32 \
         -Wall -Wextra -nostdlib -g -std=gnu99 -O0 \
         -I$(SRC_DIR)/include -I$(SRC_DIR)/libs -I$(SRC_DIR)/kernel \
         -I$(SRC_DIR)/sys -I$(SRC_DIR)/apps -I$(SRC_DIR)/drivers \
         -w -fno-stack-check

ASMFLAGS = -f elf32 -g
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

KERNEL = kernel.bin
ISO = lumenos.iso
LIMINE_DIR = limine

# Поиск файлов в src и её поддиректориях
C_FILES = $(shell find $(SRC_DIR) -name "*.c" -not -path "./$(LIMINE_DIR)/*")
ASM_FILES = $(SRC_DIR)/boot/boot.asm $(shell find $(SRC_DIR)/sys -name "*.asm" 2>/dev/null)

# Преобразование путей к объектным файлам в build директории
C_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_FILES))
ASM_OBJS = $(patsubst $(SRC_DIR)/%.asm, $(BUILD_DIR)/%.o, $(ASM_FILES))
OBJS = $(C_OBJS) $(ASM_OBJS)

.PHONY: all clean run kernel iso

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
		echo "ERROR: limine.conf not found"; exit 1; \
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

# Правило для компиляции C файлов
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

# Правило для ассемблерных файлов
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	@$(ASM) $(ASMFLAGS) $< -o $@

run: $(ISO)
	$(QEMU) -cdrom $(ISO) -serial stdio -vga std \
	-audiodev sdl,id=audio0 -machine pcspk-audiodev=audio0 \
	-drive file=disk.img,format=raw,if=ide,index=0,media=disk \
	-m 64M

clean:
	@find . -type f \( -name "*.o" -o -name "*.d" \) -delete
	@rm -rf $(BUILD_DIR) $(KERNEL) $(ISO) iso_root
	@echo "Cleaned build directory"

kernel: $(KERNEL)
iso: $(ISO)