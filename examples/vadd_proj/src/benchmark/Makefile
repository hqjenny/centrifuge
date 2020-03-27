CFLAGS += --std=gnu99
ifeq ($(ARCH),riscv)
	CC=riscv64-unknown-linux-gnu-gcc
	CFLAGS += -DRISCV
else
	CC=gcc
endif

LDFLAGS=-lvadd_tl -lvadd_rocc -L../../../libcf/lib 
INCLUDE=-I../../../../libcf/include 
ifdef $(CF_ACCEL)
# Use centrifuge generated accelerators
LDFLAGS += -L../../centrifuge_wrappers/rocc0_vadd_vadd/ -L../../centrifuge_wrappers/tl0_vadd_tl_vadd/
INCLUDE += -I../../centrifuge_wrappers/rocc0_vadd_vadd/ -I../../centrifuge_wrappers/tl0_vadd_tl_vadd/
else
# Use non-accelerated versions
LDFLAGS += -L../hls/vadd_rocc/ -L../hls/vadd_tl/
INCLUDE += -I../hls/vadd_rocc/ -I../hls/vadd_tl/
endif

vadd: vadd.o
	$(CC) $(CFLAGS) $(INCLUDE) $^ $(LDFLAGS) -o $@

%.o: %.c
	  $(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f *.o
	rm -f vadd
