VERBOSE= 1
TARGET ?=aes_tl

FUNC=encrypt
LDFLAGS=
CFLAGS=

POSTFIX=
ifeq ($(CUSTOM_INST), 1)
  POSTFIX=.inst
	CFLAGS+=-DCUSTOM_INST
endif 

ifeq ($(CUSTOM_DRIVER), 1)
  POSTFIX=.driver
	CFLAGS+=-DCUSTOM_DRIVER
endif 

include ../Makefile
