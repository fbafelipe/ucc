UCC=../../build/ucc
CFLAGS=-I ../../include

all: test1.vm test2.vm test3.vm test4.vm test5.vm test6.vm test7.vm test8.vm test9.vm

%.vm: %.c
	$(UCC) $< $(CFLAGS) -o $@

clean:
	rm -f *.vm
