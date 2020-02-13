CC=gcc
CFLAGS=--std=gnu99 -DCF_LINUX

LIB=libcf.a
LIB_SRCS=centrifuge.c os_utils.c
LIB_OBJS=$(patsubst %.c,%.o,$(LIB_SRCS))

TEST=libcf_test
TEST_SRCS=libcf_test.c
TEST_OBJS=$(patsubst %.c,%.o,$(TEST_SRCS)) $(LIB)

INCLUDES=-I../include

all: $(LIB) $(TEST)

$(TEST): $(TEST_OBJS) $(LIB)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(LIB): $(LIB_OBJS)
	ar rcs $@ $^

%.o: %.c
	  $(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f *.o
	rm -f $(TEST) 
	rm -f $(LIB)

print-%  :
	@echo $* = $($*)
