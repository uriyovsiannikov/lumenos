#!/bin/bash

echo "Fixing Limine ISO structure..."

# Удаляем старый ISO если есть
rm -f os.iso

# Создаем новую структуру
rm -rf iso_root
mkdir -p iso_root

# Копируем ядро
cp kernel.bin iso_root/

# Копируем limine-bios.sys в корень
cp limine/limine-bios.sys iso_root/

# Создаем limine.cfg в корне
cat > iso_root/limine.cfg << 'EOF'
:LumenOS v0.4
    PROTOCOL=multiboot1
    KERNEL_PATH=boot:///kernel.bin
    CMDLINE=console=ttyS0

:LumenOS (Recover Mode)
    PROTOCOL=multiboot1
    KERNEL_PATH=boot:///kernel.bin
    CMDLINE=secured console=ttyS0
EOF

# Создаем ISO
xorriso -as mkisofs \
    -b limine-bios.sys \
    -no-emul-boot \
    -boot-load-size 4 \
    -boot-info-table \
    --protective-msdos-label \
    iso_root \
    -o os.iso

# Устанавливаем Limine
if [ -f "limine/limine" ]; then
    chmod +x limine/limine
    limine/limine bios-install os.iso
elif [ -f "limine/limine.exe" ]; then
    wine limine/limine.exe bios-install os.iso
fi

echo "ISO fixed: os.iso"
ls -la os.iso
