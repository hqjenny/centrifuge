# Example Centrifuge Project: Vector Addition
This example project accelerates a simple vector addition operation using both
RoCC and TileLink. The benchmark compares these against the baseline
implementations. 

## Quick Start
This project is designed to run under many different configurations so you can
compare across platforms. This includes x86, riscv-linux, and riscv-baremetal,
as well as accelerated and non-accelerated versions.

### Generating the Hardware and Software Wrapper Sources
To begin, you will need to use Centrifuge to convert the HLS vector add code
into hardware sources and software wrappers. 

    $ make hardware
    $ make wrappers

These make targets are thin wrappers around the `centrfigure generate_hw` and
`centrifuge generate_sw` commands, with a bit of extra support for this
project's build system.

You should now have a `./centrifuge_wrappers` directory that contains generated
wrapper code. Compiling the generated hardware sources is out of scope for this
readme, but is not needed to get the software built.

### Directly building and running baremetal
We will start by building the project to run bare-metal without hardware acceleration:

    $ ./build.py --arch riscv --os baremetal

You should now have the binary `./build/vadd\_riscv\_baremetal`. Since this version doesn't require any special hardware, we can run it in spike to make sure the rest of our application works:

    $ spike ./build/vadd\_riscv\_baremetal
    Test Success!

### Using FireMarshal to test Linux
Building the linux-based tests manually follows the same procedure as above,
but running is a bit more complicated. To help with building and running, we
use the FireMarshal build management tool from Chipyard. FireMarshal workloads
also make working with FireSim easier. You can learn more
[here](https://firemarshal.readthedocs.io/en/latest/). Make sure the marshal
executable is on your path and run the following command:

    $ cd workloads
    $ marshal test vadd_linux.json
    ...
    SUCCESS: All Tests Passed (0 tests skipped)

There will be a bit of output and a delay after starting the test, but you
should be presented with a success message. You can also test bare-metal workloads the same way:

    $ cd workloads/
    $ marshal test --spike vadd_baremetal.json 

### Working with accelerated versions
We also include FireMarshal workloads for the accelerated versions. These won't
work with the `marshal test` command since they require special hardware
generated with Centrifuge, so we'll have to run them in FireSim. You will first need to install the workloads to FireSim using FireMarshal:

    $ marshal install vadd_baremetal_accel.json vadd_linux_accel.json

You can then refer to the FireSim and Centrifuge documentation to see how to
launch the install workloads in FireSim on a Centrifuge-generated AMI. 

## Directory Structure
* src/ - All application sources
  * benchmark/ - The application that actually uses the accelerated function. In this case it simply tests for correctness.
  * hls/ - These sources contain the HLS C code implementing vadd in RoCC and TL
* workloads/ - FireMarshal workloads for interacting with the application.
* build/ - This directory is dynamically generated when you run build.py. It contains all generated outputs (like libraries).
* centrifuge\_wrappers/ - This directory is dynamically generated when you build the centrifuge wrappers.

## Project Details
This project works by compiling the different subprojects into static libraries
(that are installed to the ./build/ directory) and linking the benchmark code
against it. The build.py script helps automate this process, but you are
encouraged to read through the Makefiles under src/ to understand what's going
on. Most of the magic is in common.mk that shows what flags are used for each
build configuration.

The libraries that we build are:
* libvadd\_rocc/tl\_$(CONFIG)\_accel.a - This contains the compiled accelerator
  wrappers from `centrifuge\_wrappers`.
* libvadd\_rocc/tl\_$(CONFIG).a - These libraries contain the non-accelerated
  vadd implementations (from `src/hls/*`) 
* libcf\_$(CONFIG).a - This helper library is included with centrifuge (at
  `centrifuge/sw_aux/lib`) and is usually built when you first setup
  centrifuge.
* libriscvbm.a - This helper library (at `centrifuge/sw\_aux/lib/`) contains
  all the support sources for running baremetal code. This is optional but
  makes building baremetal applications easier.

The non-accelerated sources in `src/hls/*` contain a function that matches the
wrapper's signature (e.g. `src/hls/vadd_tl/vadd_tl.h` contains
`vadd_tl_cf_em`). This makes it easy for the benchmark to switch between
accelerated and non-accelerated implementations. The wrapped versions will be
named, e.g. `vadd_tl_cf_accel` or `vadd_rocc_cf_accel`.

The benchmark at `src/benchmark/vadd.c` generates some test vectors, and runs
them against either the baseline or accelerated version (based on the
`CF_ACCEL` flag set by build.py). It then compares the output to a trivial
implementation of vadd to test correctness.
