all: build_sh4 build_arm7

build_sh4: sh4/arm7_prog.h
	make -C sh4

build_arm7:
	make -C arm7

sh4/arm7_prog.h: build_arm7 embed_c_code.py
	./embed_c_code.py -i arm7/armwrestler-ds.bin -o sh4/arm_prog.h -t arm7_program -h arm_prog_h_

clean:
	rm -f sh4/arm7_prog.h
	make -C sh4 clean
	make -C arm7 clean
