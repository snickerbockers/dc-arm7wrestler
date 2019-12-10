all:
	make -C sh4
	make -C arm7

clean:
	make -C sh4 clean
	make -C arm7 clean
