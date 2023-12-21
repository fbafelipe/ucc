UCC=../../build/ucc
CFLAGS=-E -I /usr/include

all: test1.output test2.output test3.output test4.output test5.output test6.output test7.output



%.output: %.c
	$(UCC) $< -o $@ $(CFLAGS)

clean:
	rm -f *.output
