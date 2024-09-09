# Centrifuge - A Unified Approach to Generate RISC-V Accelerator SoC 

## 1. Centrifuge Setup 
1) Set up the Chipyard repo following the instruction [here](https://chipyard.readthedocs.io/en/latest/Chipyard-Basics/Initial-Repo-Setup.html#initial-repository-setup). For more Chipyard related info, visit the Chipyard page (https://chipyard.readthedocs.io/en/latest/). Please check out Chipyard version 1.8.1. 

2) (OPTIONAL for FPGA-accelerated simulation) Set up the FireSim repo. 
For more FireSim related info, please refer to FireSim’s documentation (https://docs.fires.im/en/latest/index.html). 
For using FireSim in Chipyard, refer to (https://chipyard.readthedocs.io/en/latest/Simulation/FPGA-Accelerated-Simulation.html). 

3) Clone Centrifuge and set up its dependencies. 
Git clone the current repo to the Chipard tools directory.
```
pushd chipyard/tools && git clone -b python-dev-new git@github.com:hqjenny/centrifuge.git && popd
```
Set up riscv-pk for accelerator calls and applies patches to existing tools. 
```
cd tools/centrifuge && source scripts/hls-setup-a-machine.sh && cd ../..
pushd tools/centrifuge && pip3 install -r python-requirements.txt && popd
```

## 2. Running Centrifuge
1) Before running Centrifuge, source the env setup scripts.
It sets an environmental variable `RDIR` to the root directory of the Chipyard, which we used to construct paths in the scritps. 
```
source tools/centrifuge/env.sh
```
 
### 2.1 Vector Add Example 
Run Centrifuge to generate the accelerator SoC defined in `vadd_soc.json`.
```
cd $RDIR/tools/centrifuge/deploy
./centrifuge generate_hw -c ../examples/vadd_proj/vadd_soc.json
```
This also generates the sw helper functions to invoke the accelerator. The generated sw wrapper `accel_wrapper.c` and `accel_wrapper.h`is under the hardware path  `$RDIR/tools/centrifuge/examples/vadd_proj/centrifuge_wrappers/`. 

4) Software Compilation

Run the following command to invoke compilation for bare-metal. 
```
./centrifuge generate_sw -c ../examples/vadd_proj/vadd_soc.json 
```
We currently have the vadd RoCC example code and Makefile under example directory and are working on migrating the compilation flow from perl to python.
```
cd $RDIR/tools/centrifuge/examples/vadd_proj && make
```
The generated `vadd.bm.rv` is a software only reference code for vadd and `vadd.bm_accel.rv` is the baremetal code for calling the RoCC vadd accelerator

5) Run VCS/Verilator Simulation 
To run VCS simulation, 
```
./centrifuge run_vcs -c ../examples/vadd_proj/vadd_soc.json 
```
Replace `run_vcs` with `run_verilator` for Verilator runs. 
This command generates a simulation executable called `simv-example-HLSRocketConfig-debug` under `$RDIR/sims/vcs/`. 
This executable is a simulator that has been compiled based on the design that was built. 

You can then use this executable to run any compatible RV64 code. 
For instance, to invoke the accelerator in bare-metal software, run:
```
cd $RDIR/sims/vcs/
./simv-example-HLSRocketConfig-debug <sw binary>
```
To run the simulation using Chipyard, run:
```
cd $RDIR/sims/vcs/
make run-binary-debug CONFIG=HLSRocketConfig BINARY=<sw binary>
```
