VERBOSE=1
TARGET=vadd_tl
FUNC=vadd
LDFLAGS=
CFLAGS=
SRC=/scratch/schwarzem/chipyard-fork-cs267/tools/centrifuge/examples/vadd_proj/src/hls/vadd_tl/vadd_tl.c /scratch/schwarzem/chipyard-fork-cs267/tools/centrifuge/examples/vadd_proj/src/hls/vadd_tl/accel.c /scratch/schwarzem/chipyard-fork-cs267/tools/centrifuge/examples/vadd_proj/src/hls/vadd_tl/accel.h

ifeq ($(CUSTOM_INST), 1)
	CFLAGS+=-DCUSTOM_INST
endif

ifeq ($(CUSTOM_DRIVER), 1)
	CFLAGS+=-DCUSTOM_DRIVER
endif

ifeq ($(LLVM), 1)
	ACCEL ?=0
	include ../../scripts/sw_aux/makefiles/Makefile.llvm.in
else
ifeq ($(GCC), 1)
	include ../../scripts/sw_aux/makefiles/Makefile.gcc.in
else
	include ../../scripts/sw_aux/makefiles/Makefile.bm.in
endif
endif
