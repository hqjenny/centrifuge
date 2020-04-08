CFLAGS += --std=gnu99

ifeq ($(ARCH),riscv)

ifeq ($(HOST),baremetal) # RISCV BAREMETAL
SUFFIX=riscv_bare
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

ifdef $(CF_ACCEL)
	SUFFIX=$(SUFFIX)_accel
endif
