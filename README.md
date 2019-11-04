# Centrifuge - A Unified Approach to Generate RISC-V Accelerator SoC 

## 1. Centrifuge Setup 
1) Set up the Chipyard repo. For more Chipyard related info, 
please refer to the Chipyard Tutorial (https://chipyard.readthedocs.io/en/latest/) 
```
git clone https://github.com/hqjenny/chipyard.git
cd chipyard
./scripts/init-submodules-no-riscv-tools.sh
./scripts/build-toolchains.sh #
source ./scripts/env.sh
```

2) (OPTIONAL for FPGA-accelerated simulation) Set up the FireSim repo. 
For more FireSim related info, please refer to FireSim’s documentation (https://docs.fires.im/en/latest/index.html). 
For using FireSim in Chipyard, refer to (https://chipyard.readthedocs.io/en/latest/Simulation/FPGA-Accelerated-Simulation.html). 

```
./scripts/firesim-setup.sh --fast
pushd sims/firesim
source sourceme-f1-manager.sh
popd
```

3) Set up the Centrifuge dependencies. 
This sets up the required scripting packages and applies patches to existing tools. 
```
source tools/centrifuge/scripts/source hls-setup.sh
```

## 2. Running Centrifuge
1) Before running Centrifuge, source the env setup scripts.
It sets an environmental variable `RDIR` to the root directory of the Chipyard, which we used to construct paths in the scritps. 
```
source tools/centrifuge/env.sh
```
 
### 2.1 Vector Add Example 
1) Source code format
2) Configuration format 

3) Run Centrifuge to generate the accelerator SoC defined in `accel.json`.
```
cd $RDIR/tools/centrifuge/scripts
perl generate_soc.pl accel.json accel
```
The generated sw wrapper `accel_wrapper.c` and `accel_wrapper.h`is under the hardware path  `$RDIR/generators/accel/hls_vadd_tl_vadd/src/main/c`. The makefile for baremetal is compilation 
is copied to `$RDIR/generators/accel/Makefile.bm.in`. The makefile for linux is copied to `$RDIR/generators/accel/Makefile.gcc.in`. The postfix of the bare-metal program is `.bm.rv` and `.bm_accel.rv`
for programs with or without using the accelerator. Run `make` will generate both, while `make accel` generates only `.bm_accel.rv`. The postfix of generated linux program is `.rv`.

4) Run VCS/Verilator Simulation 
4.1) Run Baremetal SW
5) Run FPGA Simulation 
5.1) Generate FireSim Image 
5.2) Run Baremetal SW
5.3) Run Linux SW
