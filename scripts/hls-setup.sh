#!/bin/bash

# Run this script on a fresh clone to initialize any patches or tweaks needed
# This assumes you've already fully set up firesim normally (at least run build-setup.sh)

# go to top-level dir
pushd $(git rev-parse --show-toplevel)
export RDIR=$(git rev-parse --show-toplevel)

# Set RDIR (path to top of firesim repo) globally since our scripts use it
#sed -i 's/RDIR=/export RDIR=/' sourceme-f1-manager.sh
#source sourceme-f1-manager.sh

# modify the pk source code to enable pk for custom instructions
pushd $RDIR/toolchains/riscv-tools/riscv-pk
git apply $RDIR/tools/centrifuge/patches/riscv-pk.patch
popd

#pushd $RDIR
#./scripts/build-toolchains.sh
#popd

# modify the midas F1Transform to generate clock gating buffers for blackbox modules:
# pushd $RDIR/sim/midas/src/main/scala/midas/passes
# git apply $RDIR/hls/patches/Fame1Transform.scala.patch
# popd

# Patch Generator.scala and Rocketchip Fragmentator code for HLS
# pushd $RDIR/sim/src/main/scala
# git apply $RDIR/hls/patches/Generator.scala.patch
# cd $RDIR/sim/target-rtl/firechip/rocket-chip/src/main/scala
# git apply $RDIR/hls/patches/Fragmenter.scala.patch
# popd

# Setup all the auxiliary makefiles for RISC-V baremetal compilation for print statement and etc.
#cp $RDIR/tools/centrifuge/makefiles/* $RDIR/sim/target-rtl/firechip
pushd $RDIR/tools/centrifuge/scripts/sw_aux/bm_linker_scripts
make
popd

# Enable custom instruction and loadable kernel modules on riscv-linux
#pushd $RDIR/sw/firesim-software/riscv-linux
#touch arch/riscv/include/asm/module.h && touch arch/riscv/kernel/module-sections.c && touch arch/riscv/kernel/module.lds
#git apply $RDIR/hls/patches/riscv-linux.patch
#popd
#
pushd $CL_DIR/verif/scripts
git apply $RDIR/tools/centrifuge/patches/XSim_Makefile.patch
touch top.vivado.vhd.f
popd

## Add Linux static memory allocator for the accelorator
#cp $RDIR/hls/sw/riscv-linux/hls_mmap_static.c  $RDIR/sw/firesim-software/riscv-linux/mm/
#echo "obj-y += hls_mmap_static.o" >> $RDIR/sw/firesim-software/riscv-linux/mm/Makefile
#
## Only on millennium machines:
#if [ "$(domainname)" == "mill" ]; then
#  # Enable VCS simluation with BUFGCE libraries.
#  pushd $RDIR/sim/midas/src/main/cc
#  git apply $RDIR/hls/patches/VCS_Makefile.patch
#  popd
#
#  # Setup perl modules
#  cpan JSON
#  cpan Tie::File::AsHash
#
#  # Get vivado license stuff in the sourceme
#  echo "
#source /ecad/tools/fpga.bashrc
#source /ecad/tools/xilinx/Vivado/2017.1/settings64.sh > /dev/null" >> $RDIR/sourceme-f1-manager.sh
#fi
#
# Only on AWS FPGA AMI machines (see firesim/build-setup-nolog.sh for how this trick works):
if wget -T 1 -t 3 -O /dev/null http://169.254.169.254/; then
  # # Update Makefiles for Xsim to support VHDL simluation.
  pushd $CL_DIR/verif/scripts
  git apply $RDIR/hls/patches/XSim_Makefile.patch
  touch top.vivado.vhd.f
  popd

  # Setup perl modules
  sudo yum install -y cpan
  sudo cpan JSON
  sudo yum install -y perl-Tie-IxHash
fi
