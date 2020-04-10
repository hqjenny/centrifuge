CFLAGS=-g -O0 --std=gnu99 

LIB_SRCS=centrifuge.c os_utils.c

TEST_SRCS=libcf_test.c

INCLUDES=-I../../include

ifeq ($(ARCH),riscv)

ifeq ($(HOST),baremetal) # RISCV BAREMETAL
SUFFIX=riscv_baremetal
CC=riscv64-unknown-elf-gcc
CFLAGS+=-mcmodel=medany -fno-common -fno-builtin-printf -DRISCV -static

else #RISCV LINUX
SUFFIX=riscv_linux
CC=riscv64-unknown-linux-gnu-gcc
CFLAGS += -static -DRISCV -DCF_LINUX
endif

else #x86 LINUX
SUFFIX=x86
CC=gcc
CFLAGS += -DCF_LINUX
endif

ifdef ACCEL
CFLAGS += -DCF_ACCEL
SUFFIX:=$(SUFFIX)_accel
endif

LIB=libcf_$(SUFFIX).a
LIB_OBJS=$(patsubst %.c,%.$(SUFFIX).o,$(LIB_SRCS))
TEST=libcf_test.$(SUFFIX)
TEST_OBJS=$(patsubst %.c,%.$(SUFFIX).o,$(TEST_SRCS)) $(LIB)

.PHONY: all
ifeq ($(HOST),baremetal)
all: $(LIB)
else
all: $(LIB) $(TEST)
endif

$(TEST): $(TEST_OBJS) $(LIB)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

.PHONY: lib
lib: $(LIB)

$(LIB): $(LIB_OBJS)
	ar rcs $@ $^

%.$(SUFFIX).o: %.c
	  $(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: install
install: $(LIB)
	cp $(LIB) ../../lib/

.PHONY: clean
clean:
	rm -f *.o
	rm -f libcf_test.x86 libcf_test.riscv_linux libcf_test.riscv_bare
	rm -f libcf*.a

print-%  :
	@echo $* = $($*)
