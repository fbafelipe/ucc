UCC=../../../build/ucc
CFLAGS=-I ../../../include

TARGET=prime.vm
OBJECTS=main.asm search.asm dump.asm

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(UCC)  $(OBJECTS) -o $(TARGET)

%.asm: %.c
	$(UCC) $< $(CFLAGS) -o $@ -c

clean:
	rm -f $(TARGET)
	rm -f *.asm
