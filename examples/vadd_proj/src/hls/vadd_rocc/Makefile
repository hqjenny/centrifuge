CC=gcc
CFLAGS=--std=c99

LIB=libvadd_rocc.a
TEST=vadd_rocc_test

all: $(LIB) $(TEST)

$(TEST): vadd_rocc.o vadd_rocc_test.o
	$(CC) $^ -o $@

$(LIB): vadd_rocc.o
	ar rcs $@ $^

%.o: %.c
	  $(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
	rm -f $(TEST) 
	rm -f $(LIB)
