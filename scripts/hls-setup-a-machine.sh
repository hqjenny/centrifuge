#!/bin/bash

# Run this script on a fresh clone to initialize any patches or tweaks needed
# This assumes you've already fully set up firesim normally (at least run build-setup.sh)

# go to top-level dir
pushd $(git rev-parse --show-toplevel)
export RDIR=$(git rev-parse --show-toplevel)/../../

if [ -d /ecad/tools ]; then
    source /ecad/tools/vlsi.bashrc 
    export VCS_HOME=/ecad/tools/synopsys/vcs/P-2019.06
    export VCS_LIC_EXPIRE_WARNING=0
    export XILINXD_LICENSE_FILE="2200@sunv40z-1.eecs.berkeley.edu"
    source /ecad/tools/xilinx/Vivado/2018.2/settings64.sh > /dev/null
fi

# Setup all the auxiliary makefiles for RISC-V baremetal compilation for print statement and etc.
pushd $RDIR/tools/centrifuge/scripts/sw_aux/bm_linker_scripts
make
popd

