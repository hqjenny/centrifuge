ifneq ($(KERNELRELEASE),)

obj-m += centrifuge_mmap.o

else

# The default assumes you cloned this as part of firesim-software (FireMarshal)
LINUXSRC=../linux

KMAKE=make -C $(LINUXSRC) ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- M=$(PWD)

centrifuge_mmap.ko: centrifuge_mmap.c
	$(KMAKE)

clean:
	$(KMAKE) clean

endif
