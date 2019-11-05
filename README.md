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
./tools/centrifuge/scripts/hls-setup.sh
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
This also generates the sw helper functions to invoke the accelerator. The generated sw wrapper `accel_wrapper.c` and `accel_wrapper.h`is under the hardware path  `$RDIR/generators/accel/hls_vadd_tl_vadd/src/main/c`. The makefile for baremetal is compilation 
is copied to `$RDIR/generators/accel/Makefile.bm.in`. The makefile for linux is copied to `$RDIR/generators/accel/Makefile.gcc.in`. The postfix of the bare-metal program is `.bm.rv` and `.bm_accel.rv`
for programs with or without using the accelerator. Run `make` will generate both, while `make accel` generates only `.bm_accel.rv`. The postfix of generated linux program is `.rv`.

4) Software Compilation

Run the following command to invoke compilation for bare-metal. 
```
perl generate_soc.pl accel.json compile_sw_bm
```

5) Run VCS/Verilator Simulation 
To run VCS simulation, 
```
perl generate_soc.pl accel.json run_vcs
```
Replace `run_vcs` with `run_verilator` for Verilator runs. 
This command generates a simulation executable called `simv-example-HLSRocketConfig-debug` under `$RDIR/sims/vcs/`. 
This executable is a simulator that has been compiled based on the design that was built. 

You can then use this executable to run any compatible RV64 code. 
For instance, to invoke the accelerator in bare-metal software for `vadd_tl` accelerator, run:
```
cd $RDIR/sims/vcs/
./simv-example-HLSRocketConfig-debug $RDIR/generators/accel/hls_vadd_tl_vadd/src/main/c/vadd_tl.bm_accel.rv
```

5) Run FPGA Simulation 
a) Following the FireSim instrucitons to set up the manager instance here
(https://docs.fires.im/en/latest/Initial-Setup/Setting-up-your-Manager-Instance.html). 
Commands `aws configure` and `firesim managerinit` should be run for the setup.
Then the user should set up S3 buckets name in `$RDIR/sims/firesim/deploy/config_build.ini` 
following the instructions here (https://docs.fires.im/en/latest/Building-a-FireSim-AFI.html). 


b) Generate FireSim Image 
First, run the following command to generate a new accelerator configuration in FireSim with `DESIGN=FireSimTopWithHLS`, `TARGET_CONFIG=HLSFireSimRocketChipConfig` and `PLATFORM_CONFIG=BaseF1Config_F90MHz`:

```
perl generate_soc.pl accel.json f1_scripts
```
This commmand also sets up custom F1 scripts to compile the design with accelerators. 

Then we need configure the FireSim build receipes. 
First, append the following build recipes to the `$RDIR/sims/firesim/deploy/config_build_recipes.ini`
```
[firesimhls-singlecore-no-nic-l2-lbp]
DESIGN=FireSimTopWithHLSNoNIC
TARGET_CONFIG=HLSFireSimRocketChipConfig
PLATFORM_CONFIG=BaseF1Config_F90MHz
instancetype=c5.4xlarge
deploytriplet=None
```
Then, add `firesimhls-singlecore-no-nic-l2-lbp` to the `[builds]` and `[agfistoshare]` sections in file 
`$RDIR/sims/firesim/deploy/config_build.ini`. 

Lastly, run `firesim buildafi` to start building the FPGA image.
To understand how FireSim manager works, please refer to (https://docs.fires.im/en/latest/Building-a-FireSim-AFI.html)

d) Run Baremetal SW
With the `perl generate_soc.pl accel.json accel` command, we also generate JSON configurations file for fireMarshal to build  software workload to run on FireSim. To run the `vadd` acclerator defined in `vadd.json`, run fireMarshal to generate the workload: 
```
$RDIR/sims/firesim/sw/firesim-software/marshal build $RDIR/generators/accel/hls_vadd_vadd/src/main/c/vadd-bare-sw-bm_accel.json
$RDIR/sims/firesim/sw/firesim-software/marshal install $RDIR/generators/accel/hls_vadd_vadd/src/main/c/vadd-bare-sw-bm_accel.json
```

In file `$RDIR/sims/firesim/deploy/config_runtime.ini`, change the default configuration 
`defaulthwconfig` to `firesimhls-singlecore-no-nic-l2-lbp` and the workload `workloadname` to `vadd-bare-sw-bm_accel.json`.

e) Run Linux SW
