ARCH=riscv
CROSS_COMPILE=riscv64-unknown-linux-gnu-
KDIR:=/scratch/qijing.huang/firesim/sw/firesim-software/riscv-linux/ 

#obj-m :=hello.o
obj-m +=hls_mmap.o

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
