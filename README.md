**32 Bit OS written in C & Assembly**

*Features:*
- Basic I/O system -- DONE
- Console -- DONE
- File System -- DONE
- Memory Managment -- DONE
- Multitasking -- IN PROGRESS
- Panic handlers -- IN PROGRESS
- Power Managment -- DONE
- System logging -- DONE
- Timers -- DONE
- Simple BASIC -- DONE
- PCI devices detecting -- DONE
- Ring Buffer -- DONE

*Drivers:*
- Disk driver -- DONE
- Serial driver -- DONE
- Speaker driver -- DONE
- Vga (graphics) -- DONE
- Floppy disk -- UNSTABLE

*Included apps:*
- ASCII table
- Calculator
- Clock
- Memory Map
- Spreadsheet editor
- System info
- Text Editor

**Building & Running**

To build *LumenOS* you need to install some packages:

> sudo apt-get install nasm grub2 xorriso make mtools gcc-multilib build-essential qemu-system-x86

Then execute:

> make clean && make

And run:

> make run
