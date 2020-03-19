CFLAGS += --std=gnu99
ifeq ($(ARCH),riscv)
	CC=riscv64-unknown-linux-gnu-gcc
	CFLAGS += -DRISCV
else
	CC=gcc
endif

INCLUDE=-I../../../../../libcf/include

LIB=libvadd_rocc.a
TEST=vadd_rocc_test

all: $(LIB) $(TEST)

$(TEST): vadd_rocc.o vadd_rocc_test.o
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

$(LIB): vadd_rocc.o
	ar rcs $@ $^

%.o: %.c
	  $(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f *.o
	rm -f $(TEST) 
	rm -f $(LIB)
