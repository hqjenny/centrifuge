Set $RDIR to firesim directory, and $CDIR to current working directory by: export RDIR=<firesim>  && export CDIR=`pwd`.

1. git apply the riscv-linux.patch 
```
cd $RDIR/sw/firesim-software/riscv-linux
git apply $CDIR/riscv-linux.patch
```

2. set the KDIR in the Makefile under $CDIR to `$RDIR/sw/firesim-software/riscv-linux` and compile the loadable kernel module `bash compile.sh`. 

3. copy the current folder and global mem to the ramfs.
```
mkdir -p $RDIR/sw/firesim-software/buildroot-overlay/root
cp -r $CDIR  $RDIR/sw/firesim-software/buildroot-overlay
cp -r $CDIR/../global-mem/examples/array/ $RDIR/sw/firesim-software/buildroot-overlay
``` 

4. compile riscv-linux for spike 
```
cd $RDIR/sw/firesim-software/
./build.sh initramfs
```

5. run linux on spike, username is `root` and password is `firesim`, after logging in, load the driver to allocate 2 ^ 10 bytes of kernel buffer:
```
cd hls_mmap
insmod hls_mmap.ko page_order=10
```

6. run sp_matrix

```
cd ~/array/

mpirun -np ./sp_matrix
```


