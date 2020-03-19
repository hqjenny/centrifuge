CFLAGS += --std=gnu99
ifeq ($(ARCH),riscv)
	CC=riscv64-unknown-linux-gnu-gcc
	CFLAGS += -DRISCV
else
	CC=gcc
endif

INCLUDE=-I../../../../../libcf/include

LIB=libvadd_tl.a
TEST=vadd_tl_test


all: $(LIB) $(TEST)

$(TEST): vadd_tl.o vadd_tl_test.o
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

$(LIB): vadd_tl.o
	ar rcs $@ $^

%.o: %.c
	  $(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f *.o
	rm -f $(TEST) 
	rm -f $(LIB)
