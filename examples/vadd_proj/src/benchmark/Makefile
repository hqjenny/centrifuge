COMMON_PATH=../..
include $(COMMON_PATH)/common.mk

LDFLAGS += -L$(COMMON_PATH)/build/ -lvadd_tl_$(SUFFIX) -lvadd_rocc_$(SUFFIX) -lcf_$(SUFFIX)
INCLUDES += -I../../centrifuge_wrappers/rocc0_vadd_rocc_vadd_rocc/ -I../../centrifuge_wrappers/tl0_vadd_tl_vadd_tl/
INCLUDES += -I../hls/vadd_rocc/ -I../hls/vadd_tl/

BENCH=vadd
BENCH_SRCS=vadd.c
BENCH_OBJS=$(patsubst %.c,%.o,$(BENCH_SRCS))

$(BENCH): $(BENCH_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ $(LDFLAGS) -o $@

%.o: %.c
	  $(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f *.o
	rm -f vadd

print-%  :
	@echo $* = $($*)
