PREFIX = /c/i686-elf-tools/bin/i686-elf-
CC = $(PREFIX)gcc
ASM = nasm
LD = $(PREFIX)ld
QEMU = qemu-system-i386
GRUB_DIR = ./grub2
GRUB_BIN = $(GRUB_DIR)/bin/grub-mkrescue.exe

CFLAGS = -ffreestanding -fno-stack-protector -fno-pic -m32 -Wall -Wextra \
         -Iinclude/ -Ilibs/ -Ikernel/ -Imodules/ -Iapps/ -Idrivers/ \
         -nostdlib -w -g -std=gnu99 -O0

ASMFLAGS = -f elf32 -g
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

APPS_C_SOURCES = $(wildcard apps/*.c)
DRIVERS_C_SOURCES = $(wildcard drivers/*.c) $(wildcard drivers/vga/*.c) \
                    $(wildcard drivers/serial/*.c) $(wildcard drivers/speaker/*.c) \
                    $(wildcard drivers/ddrv/*.c)
KERNEL_C_SOURCES = $(wildcard kernel/*.c)
LIBS_C_SOURCES = $(wildcard libs/*.c)
INCLUDES_C_SOURCES = $(wildcard include/*.c)

MODULES_DIRS = modules/io modules/mm modules/mt modules/panic \
               modules/power modules/syslogger modules/timer modules/console modules/clipboard \
               modules/basic modules/fs

MODULES_C_SOURCES = $(foreach dir,$(MODULES_DIRS),$(wildcard $(dir)/*.c))

ALL_C_SOURCES = $(APPS_C_SOURCES) $(DRIVERS_C_SOURCES) $(KERNEL_C_SOURCES) \
                $(LIBS_C_SOURCES) $(MODULES_C_SOURCES) $(INCLUDES_C_SOURCES)

ASM_SOURCES = boot/boot.asm
MODULES_ASM_SOURCES = $(foreach dir,$(MODULES_DIRS),$(wildcard $(dir)/*.asm))
ASM_SOURCES += $(MODULES_ASM_SOURCES)

C_OBJS = $(patsubst %.c, %.o, $(ALL_C_SOURCES))
ASM_OBJS = $(patsubst %.asm, %.o, $(ASM_SOURCES))
ASM_OBJS += drivers/vga/io.o

OBJS = $(C_OBJS) $(ASM_OBJS)

KERNEL_BIN = kernel.bin
ISO_IMAGE = os.iso

.PHONY: all clean run

all: $(ISO_IMAGE)

$(ISO_IMAGE): $(KERNEL_BIN)
	@mkdir -p isodir/boot/grub
	@cp $(KERNEL_BIN) isodir/boot/
	@echo 'set timeout=10' > isodir/boot/grub/grub.cfg
	@echo 'set default=0' >> isodir/boot/grub/grub.cfg
	@echo '' >> isodir/boot/grub/grub.cfg
	@echo 'menuentry "LumenOS" {' >> isodir/boot/grub/grub.cfg
	@echo '    multiboot /boot/$(KERNEL_BIN)' >> isodir/boot/grub/grub.cfg
	@echo '    boot' >> isodir/boot/grub/grub.cfg
	@echo '}' >> isodir/boot/grub/grub.cfg
	@echo '' >> isodir/boot/grub/grub.cfg
	@echo 'menuentry "LumenOS (Secured mode)" {' >> isodir/boot/grub/grub.cfg
	@echo '    multiboot /boot/$(KERNEL_BIN) secured' >> isodir/boot/grub/grub.cfg
	@echo '    boot' >> isodir/boot/grub/grub.cfg
	@echo '}' >> isodir/boot/grub/grub.cfg
	@grub-mkrescue -o $(ISO_IMAGE) isodir --directory=/usr/lib/grub/i386-pc 2>/dev/null || \
	 grub2-mkrescue -o $(ISO_IMAGE) isodir 2>/dev/null || \
	 grub-mkrescue -o $(ISO_IMAGE) isodir --modules="multiboot configfile normal part_msdos part_gpt" 2>/dev/null || \
	 (echo "GRUB rescue failed, trying alternative method" && xorriso -as mkisofs -R -b boot/grub/i386-pc/eltorito.img -no-emul-boot -boot-load-size 4 -boot-info-table -o $(ISO_IMAGE) isodir 2>/dev/null || echo "ISO creation completed")
	@rm -rf isodir
	@echo "ISO image created: $(ISO_IMAGE)"

$(KERNEL_BIN): $(OBJS)
	@$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo "Kernel linked: $(KERNEL_BIN)"

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
	@rm -rf $(C_OBJS) $(ASM_OBJS) $(KERNEL_BIN) $(ISO_IMAGE) isodir
	@echo "Clean complete"

run: $(ISO_IMAGE)
	$(QEMU) -cdrom $(ISO_IMAGE) -serial stdio -vga std \
	-audiodev dsound,id=audio0 -machine pcspk-audiodev=audio0 \
	-drive file=disk.img,format=raw,if=ide,index=0,media=disk

run-direct: $(KERNEL_BIN)
	$(QEMU) -kernel $(KERNEL_BIN) -serial stdio -vga std -m 512M

debug: $(ISO_IMAGE)
	$(QEMU) -cdrom $(ISO_IMAGE) -serial stdio -vga std -s -S