include ../../../common.mk

INCLUDE=-I../../../../../libcf/include

LIB=libvadd_rocc.a
LIB_SRCS=vadd_rocc.c
LIB_OBJS=$(patsubst %.c,%.o,$(LIB_SRCS))

TEST=vadd_rocc_test
TEST_SRCS=vadd_rocc_test.c vadd_rocc.c
TEST_OBJS=$(patsubst %.c,%.o,$(TEST_SRCS))

all: $(LIB) $(TEST)

$(TEST): $(TEST_OBJS) 
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

$(LIB): $(LIB_OBJS) 
	ar rcs $@ $^

%.o: %.c
	  $(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f *.o
	rm -f libvadd_rocc.*.a
	rm -f vadd_rocc_test.riscv_linux vadd_rocc_test.riscv_bare vadd_rocc_test.x86 
