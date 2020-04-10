COMMON_PATH:=../..
include $(COMMON_PATH)/common.mk

TL_LIB=libvadd_tl.a
TL_OBJS=tl0_vadd_tl_vadd_tl_wrapper.o

ROCC_LIB=libvadd_rocc.a
ROCC_OBJS=rocc0_vadd_rocc_vadd_rocc_wrapper.o

$(TL_LIB): $(TL_OBJS) 
	ar rcs $@ $^

$(ROCC_LIB): $(ROCC_OBJS) 
	ar rcs $@ $^

%.o: %.c
	  $(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
