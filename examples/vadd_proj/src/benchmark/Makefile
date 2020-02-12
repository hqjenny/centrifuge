CC=gcc
CFLAGS=--std=c99

ifdef $(CF_ACCEL)
# Use centrifuge generated accelerators
LDFLAGS=-L../../centrifuge_wrappers/rocc0_vadd_vadd/ -lvadd_rocc -L../../centrifuge_wrappers/tl0_vadd_tl_vadd/ -lvadd_tl
INCLUDE=-I../../centrifuge_wrappers/rocc0_vadd_vadd/ -I../../centrifuge_wrappers/tl0_vadd_tl_vadd/
else
# Use non-accelerated versions
LDFLAGS=-L../hls/vadd_rocc/ -lvadd_rocc -L../hls/vadd_tl/ -lvadd_tl
INCLUDE=-I../hls/vadd_rocc/ -I../hls/vadd_tl/
endif

vadd: vadd.o
	$(CC) $(CFLAGS) $(INCLUDE) $^ $(LDFLAGS) -o $@

%.o: %.c
	  $(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f *.o
	rm -f vadd
