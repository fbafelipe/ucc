UCC=../../build/ucc
CFLAGS=-s

all: test1.output test2.output



%.output: %.c
	$(UCC) $< $(CFLAGS)

clean:
	rm -f *.output
