include ../../common.mk

LDFLAGS=-lvadd_tl_$(SUFFIX) -lvadd_rocc_$(SUFFIX) -L../../../../libcf/lib -lcf_$(SUFFIX)
INCLUDE=-I../../../../libcf/include 

ifdef $(CF_ACCEL)
# Use centrifuge generated accelerators
LDFLAGS += -L../../centrifuge_wrappers/rocc0_vadd_vadd_rocc/ -L../../centrifuge_wrappers/tl0_vadd_tl_vadd_tl/
INCLUDE += -I../../centrifuge_wrappers/rocc0_vadd_vadd_rocc/ -I../../centrifuge_wrappers/tl0_vadd_tl_vadd_tl/
else
# Use non-accelerated versions
LDFLAGS += -L../hls/vadd_rocc/ -L../hls/vadd_tl/
INCLUDE += -I../hls/vadd_rocc/ -I../hls/vadd_tl/
endif

BENCH=vadd
BENCH_SRCS=vadd.c
BENCH_OBJS=$(patsubst %.c,%.o,$(BENCH_SRCS))

$(BENCH): $(BENCH_OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) $^ $(LDFLAGS) -o $@

%.o: %.c
	  $(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f *.o
	rm -f vadd

print-%  :
	@echo $* = $($*)
