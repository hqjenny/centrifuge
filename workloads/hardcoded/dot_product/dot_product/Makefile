VERBOSE= 1
TARGET ?= dot_product
FUNC=add

LDFLAGS=
CFLAGS=

ifeq ($(CUSTOM_INST), 1)
	CFLAGS+=-DCUSTOM_INST
endif 

ifeq ($(LLVM), 1)
	ACCEL ?=0
	include ../../../../Makefile.llvm.in
else
ifeq ($(GCC), 1)
	include ../../../../Makefile.gcc.in
else
	include ../../../../Makefile.bm.in
endif
endif
