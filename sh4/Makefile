AS=sh4-linux-gnu-as
LD=sh4-linux-gnu-ld
CC=sh4-linux-gnu-gcc
OBJCOPY=sh4-linux-gnu-objcopy

all: init.bin

clean:
	rm -f init.o init.elf init.bin main.o

init.o: init.s
	$(AS) -little -o init.o init.s

init.elf: init.o main.o
	$(CC) -Wl,-e_start,-Ttext,0x8c010000 init.o main.o -o init.elf -nostartfiles -nostdlib -lgcc

init.bin: init.elf
	$(OBJCOPY) -O binary -j .text -j .data -j .bss -j .rodata  --set-section-flags .bss=alloc,load,contents init.elf init.bin

main.o: main.c #arm_prog.h
	$(CC) -c main.c -nostartfiles -nostdlib -Os
