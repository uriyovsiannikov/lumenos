#!/bin/bash

# Скрипт для создания ISO с Limine
set -e

echo "Creating ISO with Limine..."

# Создаем структуру каталогов
mkdir -p isodir/boot

# Копируем файлы
cp kernel.bin isodir/boot/
cp limine/limine-bios.sys isodir/boot/
cp limine/limine-bios-cd.bin isodir/boot/ 2>/dev/null || true

# Создаем конфигурационный файл Limine
cat > isodir/boot/limine.cfg << 'EOF'
:LumenOS
    PROTOCOL=multiboot1
    KERNEL_PATH=boot:///kernel.bin
    CMDLINE=console=ttyS0

:LumenOS (Recover Mode)
    PROTOCOL=multiboot1
    KERNEL_PATH=boot:///kernel.bin
    CMDLINE=secured console=ttyS0
EOF

# Создаем ISO
if [ -f "limine/limine-bios-cd.bin" ]; then
    xorriso -as mkisofs -b boot/limine-bios-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table \
        isodir -o os.iso
else
    # Альтернативный метод
    xorriso -as mkisofs -b boot/limine-bios.sys \
        -no-emul-boot -boot-load-size 4 -boot-info-table \
        isodir -o os.iso
fi

# Устанавливаем загрузчик Limine
if [ -f "limine/limine" ]; then
    limine/limine bios-install os.iso
elif [ -f "limine/limine.exe" ]; then
    wine limine/limine.exe bios-install os.iso
fi

echo "ISO created: os.iso"
rm -rf isodir
