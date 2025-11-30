ASM = nasm
CC = gcc
CXX = g++
LD = ld

ASMFLAGS = -f elf32
CFLAGS = -m32 -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -std=c99 -fno-stack-protector
CXXFLAGS = -m32 -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -std=c++11 -fno-exceptions -fno-rtti -fno-stack-protector -O0
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

all: ehdsb3.img

ehdsb3.img: boot.bin boot1.bin kernel3.bin
	dd if=/dev/zero of=ehdsb3.img bs=512 count=2880 2>/dev/null
	dd if=boot.bin of=ehdsb3.img conv=notrunc 2>/dev/null
	dd if=boot1.bin of=ehdsb3.img bs=512 seek=1 conv=notrunc 2>/dev/null
	dd if=kernel3.bin of=ehdsb3.img bs=512 seek=12 conv=notrunc 2>/dev/null
	@echo "Image created: ehdsb3.img"
	@echo "Kernel size:" $$(stat -c%s kernel3.bin) "bytes"

boot.bin: boot.asm
	$(ASM) -f bin boot.asm -o boot.bin

boot1.bin: boot1.asm
	$(ASM) -f bin boot1.asm -o boot1.bin

kernel3.bin: kernel3.o
	$(LD) $(LDFLAGS) -o kernel3.elf kernel3.o
	objcopy -O binary kernel3.elf kernel3.bin

kernel3.o: kernel3.cpp
	$(CXX) $(CXXFLAGS) -c kernel3.cpp -o kernel3.o

run3: ehdsb3.img
	qemu-system-x86_64 -drive format=raw,file=ehdsb3.img

debug: ehdsb3.img
	qemu-system-x86_64 -drive format=raw,file=ehdsb3.img -d int -no-reboot -no-shutdown

clean:
	rm -f *.bin *.o *.elf *.img

.PHONY: all run3 clean help debug

help:
	@echo "EHDSB (Event Horizon Dual-Stage Boot) Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all      - Build 3 kernel)"
	@echo "  run3     - Run the third microkernel"
	@echo "  debug     - Run the third microkernel with debug mode"
	@echo "  clean    - Remove all build artifacts"
	@echo ""
	@echo "Examples:"
	@echo "  make all      # Build"
	@echo "  make run3     # Run 3 kernel"
	@echo "First run:"
	@echo "  make all run3"


