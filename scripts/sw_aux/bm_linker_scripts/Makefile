AR=riscv64-unknown-elf-ar
CC=riscv64-unknown-elf-gcc
CFLAGS=-mcmodel=medany -std=gnu99 -O2 -fno-common -fno-builtin-printf -Wall
LDFLAGS=-static -nostdlib -nostartfiles -lgcc

BM_LIB_DIR=./
BM_LIB_FLAG=-lriscvbm
BM_LIB=libriscvbm.a
PROGRAMS=mnist
#default: $(BM_LIB)

#default: $(BM_LIB)

%.o: %.S
	$(CC) $(CFLAGS) -D__ASSEMBLY__=1 -c $< -o $@

%.o: %.c mmio.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BM_LIB): crt.o syscalls.o 
	$(AR) -cvq $@ *.o

clean:
	rm -f *.a *.o
