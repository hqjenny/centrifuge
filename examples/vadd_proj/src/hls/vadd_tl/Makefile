include ../../../common.mk

INCLUDE=-I../../../../../libcf/include

LIB=libvadd_tl.a
LIB_SRCS=vadd_tl.c
LIB_OBJS=$(patsubst %.c,%.$.o,$(LIB_SRCS))

TEST=vadd_tl_test
TEST_SRCS=vadd_tl_test.c vadd_tl.c
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
	rm -f libvadd_tl.*.a
	rm -f vadd_tl_test.riscv_linux vadd_tl_test.riscv_bare vadd_tl_test.x86 
