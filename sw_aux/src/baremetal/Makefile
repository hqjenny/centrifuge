AR=riscv64-unknown-elf-ar
CC=riscv64-unknown-elf-gcc
CFLAGS=-mcmodel=medany -std=gnu99 -O2 -fno-common -fno-builtin-printf -Wall
LDFLAGS=-static -nostdlib -nostartfiles -lgcc
INCLUDES=-I../../include

BM_LIB_DIR=./
BM_LIB_FLAG=-lriscvbm
BM_LIB=libriscvbm.a

.PHONY: all
all: $(BM_LIB)

install: $(BM_LIB)
	cp $(BM_LIB) ../../lib/

%.o: %.S
	$(CC) $(CFLAGS) $(INCLUDES) -D__ASSEMBLY__=1 -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BM_LIB): crt.o syscalls.o 
	$(AR) -cvq $@ *.o

clean:
	rm -f *.a *.o
