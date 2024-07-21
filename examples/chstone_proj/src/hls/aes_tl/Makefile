VERBOSE= 1
TARGET ?=aes_tl

FUNC=encrypt
LDFLAGS=
CFLAGS=

ifeq ($(CUSTOM_INST), 1)
	CFLAGS+=-DCUSTOM_INST
endif 

ifeq ($(CUSTOM_DRIVER), 1)
	CFLAGS+=-DCUSTOM_DRIVER
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
