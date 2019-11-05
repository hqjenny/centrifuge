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

#### 1) Source code format

The source code of *vadd* and *vadd_tl* is defined in `centrifuge/examples/`.
The current flow requires the user to define the function they would like to accelerate in `accel.c`. 
Its prototype definition in the header file `accel.h` must be wrapped by the following template. 
This enables proper link time behavior during the compilation.  
```
#ifdef ACCEL_WRAPPER
#include "accel_wrapper.h"
#else
int vadd(int* length_a, int* b_c);
#endif
```
#### 2) Configuration format 

Below is the SoC configuration file for the vadd example.

```
{
  "RoCC":{
    "custom0":{"pgm": "vadd", "func":"vadd"}
  },
  "TLL2":[
    {"pgm":"vadd_tl", "func": "vadd", "addr":"0x20000"}
  ]
}
```

In this configuration, we added the `vadd` function from the `vadd.c` program as a RoCC accelerator that can be invoked as `custom0` instruction. We also add a Tilelink accelerator from `vadd` function in the `vadd_tl.c` to the SoC. 
This Tilelink accelerator is mapped to the address `0x20000` and can be invoked by accessing the MMIO registers. 
Note that currently we only look at the `centrifuge/examples/${PGM}` to find the programs, where ${PGM} is the same as `pgm` defined the config file. 

For defining the RoCC accelerators, only the key `custom0` - `custom2` can be used. `custom3` RoCC Accelerator is reserved for Virtual-to-Physical Address Translator. For the Tilelink accelerators, you can specify as many accelerators as you want, as long as their MMIO addresses don't overlap with each other. 

#### 3) Centrifuge SoC Generation
Run the follow commands to generate the accelerator SoC defined in `accel.json`.
```
cd $RDIR/tools/centrifuge/scripts
perl generate_soc.pl accel.json accel
```
This also generates the sw helper functions to invoke the accelerator. The generated sw wrapper `accel_wrapper.c` and `accel_wrapper.h`is under the hardware path  `$RDIR/generators/accel/hls_vadd_tl_vadd/src/main/c`. The makefile for baremetal is compilation 
is copied to `$RDIR/generators/accel/Makefile.bm.in`. The makefile for linux is copied to `$RDIR/generators/accel/Makefile.gcc.in`. The postfix of the bare-metal program is `.bm.rv` and `.bm_accel.rv`
for programs with or without using the accelerator. Run `make` will generate both, while `make accel` generates only `.bm_accel.rv`. The postfix of generated linux program is `.rv`.

#### 4) Software Compilation

Run the following command to invoke compilation for bare-metal. 
```
perl generate_soc.pl accel.json compile_sw_bm
```

#### 5) Run VCS/Verilator Simulation 
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
#### 6) Run FPGA Simulation

##### a) Manager Setup 
Following the FireSim instrucitons to set up the manager instance here
(https://docs.fires.im/en/latest/Initial-Setup/Setting-up-your-Manager-Instance.html). 
Commands `aws configure` and `firesim managerinit` should be run for the setup.
Then the user should set up S3 buckets name in `$RDIR/sims/firesim/deploy/config_build.ini` 
following the instructions here (https://docs.fires.im/en/latest/Building-a-FireSim-AFI.html). 

##### b)Generate FireSim Image 
###### b1) FireSim Compilation

First, run the following command to generate a new accelerator configuration in FireSim with `DESIGN=FireSimTopWithHLS`, `TARGET_CONFIG=HLSFireSimRocketChipConfig` and `PLATFORM_CONFIG=BaseF1Config_F90MHz`:
```
unset FIRESIM_STANDALONE # We first need to disable firesim as a standalone module
perl generate_soc.pl accel.json f1_scripts
```
This commmand also sets up custom F1 scripts to compile the design with accelerators. 

###### b2) Configure Build Receipe 
Then we need configure the FireSim build receipes by appending the following build recipes to the `$RDIR/sims/firesim/deploy/config_build_recipes.ini`.
```
[firesimhls-singlecore-no-nic-l2-lbp]
DESIGN=FireSimTopWithHLSNoNIC
TARGET_CONFIG=HLSFireSimRocketChipConfig
PLATFORM_CONFIG=BaseF1Config_F90MHz
instancetype=c5.4xlarge
deploytriplet=None
```

###### b3) Configure HW Build
Add `firesimhls-singlecore-no-nic-l2-lbp` to the `[builds]` and `[agfistoshare]` sections in file 
`$RDIR/sims/firesim/deploy/config_build.ini`. 

###### b4) Launch Compilation for the FPGA Image
Lastly, let's start building the FPGA image.
```
firesim buildafi
```

###### b5) Configure HWDB
Once the complation of the FPGA image finishes, add the following config together with your new AGFI number
to the `$RDIR/sims/firesim/deploy/config_hwdb.ini`. 
```
[firesimhls-singlecore-no-nic-l2-lbp]
agfi=agfi-XXXXXXXXXXXXXXXXX
deploytripletoverride=None
customruntimeconfig=None
```

To understand how FireSim manager works, please refer to (https://docs.fires.im/en/latest/Building-a-FireSim-AFI.html)

##### c) Run Baremetal SW
With the `perl generate_soc.pl accel.json accel` command, we also generated
JSON configuration files for FireMarshal to build software workloads to run on
FireSim. To run the `vadd` acclerator defined in `vadd.json`, run FireMarshal
to generate the workload: 
```
$RDIR/tools/firemarshal/marshal build $RDIR/generators/accel/hls_vadd_vadd/src/main/c/vadd-bare-sw-bm_accel.json
$RDIR/tools/firemarshal/marshal install $RDIR/generators/accel/hls_vadd_vadd/src/main/c/vadd-bare-sw-bm_accel.json
```

FireMarshal's `install` command automatically generated a FireSim configuration for this workload. All that is left is to configure the simulation in FireSim. To do that, copy over the centrifuge example configuration file:
```
cp $RDIR/tools/centrifuge/configs/config_runtime.ini $RDIR/sims/firesim/deploy/
```

This file has all the configuration options FireSim needs to run a real
workload. The most important ones for our purposes are the `defaulthwconfig`
and `workloadname` options. `defaulthwconfig` is the name of the afi we
generated earlier. `workloadname` is the name of the FireMarshal workload we
installed (`vadd-bare-sw-bm_accel.json` in our case). From here, you can follow
the [official FireSim
documentation](https://docs.fires.im/en/latest/Running-Simulations-Tutorial/Running-a-Single-Node-Simulation.html)
to run a single node simulation (substituting our config_runtime.ini of
course).

##### d) Run Linux SW
See [workloads/README.md](./workloads/README.md) for instructions on how to build linux-based workloads.
To simulate these workloads, simply change the `workloadname` option in
`$RDIR/sims/firesim/deploy/config_runtime.ini` to the appropriate workload that
you installed and follow the instructions as before.
