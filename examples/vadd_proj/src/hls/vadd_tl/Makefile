CC=gcc
CFLAGS=--std=c99

LIB=libvadd_tl.a
TEST=vadd_tl_test

all: $(LIB) $(TEST)

$(TEST): vadd_tl.o vadd_tl_test.o
	$(CC) $^ -o $@

$(LIB): vadd_tl.o
	ar rcs $@ $^

%.o: %.c
	  $(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
	rm -f $(TEST) 
	rm -f $(LIB)
