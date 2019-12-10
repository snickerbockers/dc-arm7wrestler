ARM_AS=arm-linux-gnueabi-as
ARM_LD=arm-linux-gnueabi-ld
ARM_OBJCOPY=arm-linux-gnueabi-objcopy

all: armwrestler-ds.bin

clean:
	rm -rf armwrestler-ds.o armwrestler-ds.elf armwrestler-ds.bin

armwrestler-ds.o: source/armwrestler-ds.s
	$(ARM_AS) -EL -mcpu=arm7 -o armwrestler-ds.o source/armwrestler-ds.s

armwrestler-ds.elf: armwrestler-ds.o
	$(ARM_LD) -Ttext 0x00000000 armwrestler-ds.o -o arm_init.elf

armwrestler-ds.bin: armwrestler-ds.elf
	$(ARM_OBJCOPY) -O binary -j .text -j .data -j .bss -j .rodata --set-section-flags .data=alloc,load,contents arm_init.elf arm_init.bin

#arm_prog.h: arm_init.bin
#	./embed_c_code.py -i arm_init.bin -o arm_prog.h -t arm7_program -h arm_prog_h_
