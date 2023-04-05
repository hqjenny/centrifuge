# Built in support libraries that come with centrifuge
SWAUX=$(COMMON_PATH)/../../sw_aux

CFLAGS += --std=gnu99 -O0
LDFLAGS=-L$(SWAUX)/lib
INCLUDES=-I$(SWAUX)/include

ifeq ($(ARCH),riscv)

CFLAGS+=-DRISCV -static

ifeq ($(HOST),baremetal) # RISCV BAREMETAL
SUFFIX=riscv_baremetal
CC=riscv64-unknown-elf-gcc
CFLAGS+= -mcmodel=medany -fno-common -fno-builtin-printf
LDFLAGS+= -T $(SWAUX)/etc/baremetal/link.ld -static -nostdlib -nostartfiles -lgcc -lriscvbm

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

ifdef ACCEL
	CFLAGS+=-DCF_ACCEL
	SUFFIX:=$(SUFFIX)_accel
endif
