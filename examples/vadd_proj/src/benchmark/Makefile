include ../../common.mk

LDFLAGS=-lvadd_tl_$(SUFFIX) -lvadd_rocc_$(SUFFIX) -L../../../../libcf/lib -lcf_$(SUFFIX)
INCLUDE=-I../../../../libcf/include 

ifdef $(CF_ACCEL)
# Use centrifuge generated accelerators
LDFLAGS += -L../../centrifuge_wrappers/rocc0_vadd_vadd/ -L../../centrifuge_wrappers/tl0_vadd_tl_vadd/
INCLUDE += -I../../centrifuge_wrappers/rocc0_vadd_vadd/ -I../../centrifuge_wrappers/tl0_vadd_tl_vadd/
SUFFIX=$(SUFFIX).accel
else
# Use non-accelerated versions
LDFLAGS += -L../hls/vadd_rocc/ -L../hls/vadd_tl/
INCLUDE += -I../hls/vadd_rocc/ -I../hls/vadd_tl/
endif

BENCH=vadd
BENCH_SRCS=vadd.c
BENCH_OBJS=$(patsubst %.c,%.$(SUFFIX).o,$(BENCH_SRCS))

$(BENCH): $(BENCH_OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) $^ $(LDFLAGS) -o $@

%.$(SUFFIX).o: %.c
	  $(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f *.o
	rm -f vadd

print-%  :
	@echo $* = $($*)
