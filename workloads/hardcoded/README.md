# fedora_bm

## Setup 
```
yum install -y mpich-devel openblas
export PATH=/usr/lib64/mpich/bin/:$PATH && echo 'export PATH=/usr/lib64/mpich/bin/:$PATH'>> ~/.bashrc
git clone https://github.com/hqjenny/fedora_bm.git
```
## Compile work 
```
cd fedora_bm/global-mem/examples/array
mpic++ sp_matrix.cpp -g  -o sp_matrix -std=gnu++17 -I../../ -I./ -lpthread -fpermissive
cp fedora_bm/hls_mmap/hls_mmap_static.c $RDIR/firesim-software/riscv-linux/mm
echo 'obj-y+=hls_mmap_static.o' >> $RDIR/sw/firesim-software/riscv-linux/mm/Makefile
```

## Build linux images
You can roll the binaries into a linux image + rootfs. You need to build
whatever binary you would like to use first using the instructions above.

### Modifying hls.json
Right now there is only one hard-coded workload description. You'll need to
modify it for each image you'd like to build. You can create as many of the
json files as you'd like as long as you remember to change the name. The file
looks something like:
```
{
  "name" : "hls",
  "base" : "br-base.json",
  "workdir" : "hls_aes_decrypt/",
  "files" : [ ["aes", "/"] ],
  "command" : "/aes"
}
```

* "name": For right now, let's assume that the "name" field should match the
  name of the json file (I'll fix this later).
* "base": If you'd rather boot in fedora, just change this to fedora-base.json
  instead
* "workdir": point this at the directory containing the binary you want to run
* "files": the first field is the src file relative to workdir, the second is
  the destination on the rootfs
* "command": This will run automatically at boot time and then shutdown the
  system. If you don't want to system to shut down, just remove this line and
  run the command manually after it boots. 

### Building and Testing
Head over to $RDIR/sw/firesim-software for these steps. To build the image, run:
```
./sw-manager.py --workdir ../../hls/fedora_bm/ -c ../../hls/fedora_bm/hls.json build
```

This will create the following two files:
```
$RDIR/sw/firesim-software/images/hls-bin
$RDIR/sw/firesim-software/images/hls.img
```

To test the code out in qemu (assuming you built the baseline version that
doesn't use custom hardware):
```
./sw-manager.py --workdir ../../hls/fedora_bm/ -c ../../hls/fedora_bm/hls.json launch
```

### Deploying on firesim
There is an hls.json and hls/ directory under $RDIR/deploy/workloads. You can
use this per-usual. If you create more images (with different names) you'd need
to create new configs in deploy/workloads to match (there's no integration
between the firesim manager and sw-manager for now).
