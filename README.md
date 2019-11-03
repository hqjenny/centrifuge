# Centrifuge - A Unified Approach to Generate RISC-V Accelerator SoC 

## 1. Chipyard and FireSim Setup 
1) Set up AWS machine following Firesim [Setting up your Manager Instance](https://docs.fires.im/en/latest/Initial-Setup/Setting-up-your-Manager-Instance.html)

2) Before initializing Chipyard, enable make/gcc 4.x.x
```
source scl_source enable devtoolset-8 
```

3) Follow Chipyard Tutorial [Quick Start](https://chipyard.readthedocs.io/en/latest/)
and FireSim [FPGA-Accelerated-Simulation](https://chipyard.readthedocs.io/en/latest/Simulation/FPGA-Accelerated-Simulation.html)

```
git clone https://github.com/ucb-bar/chipyard.git
cd chipyard
./scripts/init-submodules-no-riscv-tools.sh
./scripts/build-toolchains.sh
source ./scripts/env.sh
```
```
./scripts/firesim-setup.sh --fast
cd sims/firesim
source sourceme-f1-manager.sh
```
Also the Centrifuge dependencies
```
cd ../../
source tools/centrifuge/scripts/source hls-setup.sh
```

4) Generate the accelerator SoC defined in `accel.json`
```
cd tools/centrifuge/scripts
source envs.sh # Set the RDIR to the Chipyard root directory
perl generate_soc.pl accel.json accel
```
5) The generated sw wrapper `accel_wrapper.c` abd `accel_wrapper.h`is under the hardware path  `$RDIR/generators/accel/hls_vadd_tl_vadd/src/main/c`. The makefile for baremetal is compilation 
is copied to `$RDIR/generators/accel/Makefile.bm.in`. The makefile for linux is copied to `$RDIR/generators/accel/Makefile.gcc.in`. The postfix of the bare-metal program is `.bm.rv` and `.bm_accel.rv`
for programs with or without using the accelerator. Run `make` will generate both, while `make accel` generates only `.bm_accel.rv`. 

The postfix of generated linux program is `.rv`.

Run Verilator Simulation 

6) Generate FireSim Image
