CFLAGS += --std=gnu99 -O0

ifeq ($(ARCH),riscv)

CFLAGS+=-DRISCV -static

ifeq ($(HOST),baremetal) # RISCV BAREMETAL
SUFFIX=riscv_bare
CC=riscv64-unknown-elf-gcc
CFLAGS+=-mcmodel=medany -fno-common -fno-builtin-printf

else #RISCV LINUX
SUFFIX=riscv_linux
CC=riscv64-unknown-linux-gnu-gcc
CFLAGS += -DCF_LINUX
endif

else #x86 LINUX
SUFFIX=x86
CC=gcc
CFLAGS += -DCF_LINUX
endif

ifdef $(CF_ACCEL)
	CFLAGS+=-DCF_ACCEL
	SUFFIX=$(SUFFIX)_accel
endif
