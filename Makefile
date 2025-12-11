PREFIX = /c/i686-elf-tools/bin/i686-elf-
CC = $(PREFIX)gcc
ASM = nasm
LD = $(PREFIX)ld
QEMU = "C:/Program Files/qemu/qemu-system-i386.exe"
OBJCOPY = $(PREFIX)objcopy

CFLAGS = -ffreestanding -fno-stack-protector -fno-pic -m32 -Wall -Wextra \
         -Iinclude/ -Ilibs/ -Ikernel/ -Imodules/ -Iapps/ -Idrivers/ \
         -DBOOTBOOT_ENABLED \
         -nostdlib -w -g -std=gnu99 -O0

ASMFLAGS = -f elf32 -g
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

# Правильный список исходных файлов
KERNEL_C_SOURCES = $(wildcard kernel/*.c)
APPS_C_SOURCES = $(wildcard apps/*.c)
DRIVERS_C_SOURCES = $(wildcard drivers/*.c) $(wildcard drivers/vga/*.c) \
                    $(wildcard drivers/serial/*.c) $(wildcard drivers/speaker/*.c) \
                    $(wildcard drivers/ddrv/*.c)
LIBS_C_SOURCES = $(wildcard libs/*.c)
INCLUDES_C_SOURCES = $(wildcard include/*.c)

MODULES_DIRS = modules/io modules/mm modules/mt modules/panic \
               modules/power modules/syslogger modules/timer modules/console modules/clipboard \
               modules/basic modules/fs

MODULES_C_SOURCES = $(foreach dir,$(MODULES_DIRS),$(wildcard $(dir)/*.c))

# Собираем все исходники и удаляем дубликаты
ALL_C_SOURCES = $(KERNEL_C_SOURCES) $(APPS_C_SOURCES) $(DRIVERS_C_SOURCES) \
                $(LIBS_C_SOURCES) $(MODULES_C_SOURCES) $(INCLUDES_C_SOURCES)
ALL_C_SOURCES := $(sort $(ALL_C_SOURCES))

ASM_SOURCES = boot/boot.asm
MODULES_ASM_SOURCES = $(foreach dir,$(MODULES_DIRS),$(wildcard $(dir)/*.asm))
ASM_SOURCES += $(MODULES_ASM_SOURCES)

C_OBJS = $(patsubst %.c, %.o, $(ALL_C_SOURCES))
ASM_OBJS = $(patsubst %.asm, %.o, $(ASM_SOURCES))
ASM_OBJS += drivers/vga/io.o

OBJS = $(C_OBJS) $(ASM_OBJS)

KERNEL_ELF = kernel.elf
BOOTBOOT_IMAGE = bootboot.img
ISO_IMAGE = os.iso

# Загружаем официальный BOOTBOOT загрузчик
BOOTBOOT_LOADER_URL = https://gitlab.com/bztsrc/bootboot/-/raw/master/bootboot/dist/bootboot_ia32.bin
BOOTBOOT_LOADER = bootboot_ia32.bin

.PHONY: all clean run download-bootboot bootboot-image run-bootboot debug

all: $(ISO_IMAGE)

# Скачиваем официальный BOOTBOOT загрузчик
download-bootboot:
	@echo "Downloading BOOTBOOT loader..."
	@if command -v curl >/dev/null 2>&1; then \
		curl -L -o "$(BOOTBOOT_LOADER)" "$(BOOTBOOT_LOADER_URL)" 2>/dev/null || echo "curl failed"; \
	elif command -v wget >/dev/null 2>&1; then \
		wget -O "$(BOOTBOOT_LOADER)" "$(BOOTBOOT_LOADER_URL)" 2>/dev/null || echo "wget failed"; \
	else \
		echo "Neither curl nor wget found. Please download manually from:"; \
		echo "https://gitlab.com/bztsrc/bootboot/-/raw/master/bootboot/dist/bootboot_ia32.bin"; \
		echo "and save as $(BOOTBOOT_LOADER)"; \
		exit 1; \
	fi
	@if [ -f "$(BOOTBOOT_LOADER)" ]; then \
		echo "BOOTBOOT loader downloaded"; \
	else \
		echo "Failed to download BOOTBOOT loader"; \
		exit 1; \
	fi

# Компиляция ядра
$(KERNEL_ELF): $(OBJS)
	@echo "Linking kernel..."
	@$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo "Kernel ELF created: $(KERNEL_ELF)"

# Создание BOOTBOOT образа
bootboot-image: download-bootboot $(KERNEL_ELF)
	@echo "Creating BOOTBOOT image..."
	@# Создаем образ: загрузчик + ядро
	@# 1. Копируем загрузчик
	@cp "$(BOOTBOOT_LOADER)" "$(BOOTBOOT_IMAGE)"
	@# 2. Дополняем до границы 4K
	@if command -v python3 >/dev/null 2>&1; then \
		python3 -c "import os; size=os.path.getsize('$(BOOTBOOT_IMAGE)'); padding=(4096 - (size % 4096)) % 4096; print(padding)" > /tmp/padding.txt; \
		PADDING=$$(cat /tmp/padding.txt); \
		rm -f /tmp/padding.txt; \
	elif command -v python >/dev/null 2>&1; then \
		python -c "import os; size=os.path.getsize('$(BOOTBOOT_IMAGE)'); padding=(4096 - (size % 4096)) % 4096; print(padding)" > /tmp/padding.txt; \
		PADDING=$$(cat /tmp/padding.txt); \
		rm -f /tmp/padding.txt; \
	else \
		PADDING=0; \
	fi; \
	if [ "$$PADDING" -gt 0 ]; then \
		echo "Adding $$PADDING bytes padding..."; \
		if command -v dd >/dev/null 2>&1; then \
			dd if=/dev/zero bs=1 count=$$PADDING >> "$(BOOTBOOT_IMAGE)" 2>/dev/null || true; \
		elif command -v head >/dev/null 2>&1; then \
			head -c $$PADDING /dev/zero >> "$(BOOTBOOT_IMAGE)" 2>/dev/null || true; \
		fi; \
	fi
	@# 3. Добавляем ядро
	@cat "$(KERNEL_ELF)" >> "$(BOOTBOOT_IMAGE)"
	@echo "BOOTBOOT image created: $(BOOTBOOT_IMAGE)"

# Создание загрузочного ISO (Multiboot через GRUB)
$(ISO_IMAGE): $(KERNEL_ELF)
	@echo "Creating ISO image (Multiboot)..."
	@mkdir -p isodir/boot/grub
	@cp $(KERNEL_ELF) isodir/boot/kernel.bin
	@echo 'menuentry "LumenOS" {' > isodir/boot/grub/grub.cfg
	@echo '    multiboot /boot/kernel.bin' >> isodir/boot/grub/grub.cfg
	@echo '    boot' >> isodir/boot/grub/grub.cfg
	@echo '}' >> isodir/boot/grub/grub.cfg
	@echo 'menuentry "LumenOS (Secured mode)" {' >> isodir/boot/grub/grub.cfg
	@echo '    multiboot /boot/kernel.bin secured' >> isodir/boot/grub/grub.cfg
	@echo '    boot' >> isodir/boot/grub/grub.cfg
	@echo '}' >> isodir/boot/grub/grub.cfg
	@if command -v grub-mkrescue >/dev/null 2>&1; then \
		grub-mkrescue -o $(ISO_IMAGE) isodir 2>/dev/null; \
	elif command -v grub2-mkrescue >/dev/null 2>&1; then \
		grub2-mkrescue -o $(ISO_IMAGE) isodir 2>/dev/null; \
	else \
		echo "GRUB not found, creating simple ISO..."; \
		cp $(KERNEL_ELF) $(ISO_IMAGE); \
	fi
	@rm -rf isodir
	@echo "ISO created: $(ISO_IMAGE)"

drivers/vga/io.o: drivers/vga/io.s
	@mkdir -p $(dir $@)
	@$(ASM) -f elf -g $< -o $@

%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	@mkdir -p $(dir $@)
	@$(ASM) $(ASMFLAGS) $< -o $@

clean:
	@rm -f $(C_OBJS) $(ASM_OBJS) $(KERNEL_ELF) $(BOOTBOOT_IMAGE) $(ISO_IMAGE) 2>/dev/null || true
	@rm -rf isodir 2>/dev/null || true
	@echo "Clean complete"

run: $(ISO_IMAGE)
	$(QEMU) -cdrom $(ISO_IMAGE) -serial stdio -vga std \
	-audiodev dsound,id=audio0 -machine pcspk-audiodev=audio0 \
	-drive file=disk.img,format=raw,if=ide,index=0,media=disk \
	-m 512M -cpu qemu32

run-bootboot: bootboot-image
	$(QEMU) -drive format=raw,file=$(BOOTBOOT_IMAGE) -serial stdio \
	-vga std -m 512M -cpu qemu32

debug: $(ISO_IMAGE)
	$(QEMU) -cdrom $(ISO_IMAGE) -serial stdio -vga std -s -S -m 512M -cpu qemu32

run-direct: $(KERNEL_ELF)
	$(QEMU) -kernel $(KERNEL_ELF) -serial stdio -vga std -m 512M -cpu qemu32

# Простая цель для проверки BOOTBOOT
check-bootboot:
	@if [ -f "$(BOOTBOOT_LOADER)" ]; then \
		echo "BOOTBOOT loader found: $(BOOTBOOT_LOADER)"; \
		ls -la "$(BOOTBOOT_LOADER)"; \
	else \
		echo "BOOTBOOT loader not found. Run 'make download-bootboot'"; \
	fi

# Быстрая сборка только ядра
kernel-only: $(KERNEL_ELF)
	@echo "Kernel built: $(KERNEL_ELF)"

help:
	@echo "Available targets:"
	@echo "  all              - Build ISO with GRUB (default)"
	@echo "  bootboot-image   - Build BOOTBOOT image"
	@echo "  download-bootboot - Download BOOTBOOT loader"
	@echo "  run              - Run ISO in QEMU"
	@echo "  run-bootboot     - Run BOOTBOOT image in QEMU"
	@echo "  run-direct       - Run kernel directly in QEMU"
	@echo "  debug            - Run ISO in QEMU with GDB support"
	@echo "  clean            - Clean all build files"
	@echo "  check-bootboot   - Check if BOOTBOOT loader exists"
	@echo "  kernel-only      - Build only the kernel"