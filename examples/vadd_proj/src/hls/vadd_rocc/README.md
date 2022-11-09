# ROCC Vector Add
This folder contains the C implementation of vector addition that is fed into
the HLS tools to generate the RoCC accelerator. The HLS pragmas here are
different than those for tilelink, but the non-HLS parts are largely identical
to the tilelink code. When fed to a regular C compiler, this code should run
just fine natively.

Unlike TileLink, RoCC accelerators do not require any special handling to run
bare-metal or on Linux.

## Targets and Use-cases

* **Hardware generation**: vadd\_rocc.c is used as input to HLS tools in order to
  generate the accelerator. We don't handle that here though, you need to use
  the centrifuge `generate\_hw` command for that (out of scope for this README).
* **libvadd\_rocc.a**: For software testing and baseline comparisons, we can
  compile vadd\_rocc.c into a standalone C library that runs entirely in SW (no
  special hardware). Benchmarks and tests should link against this library (and
  vadd.h) to use the function.
* **vadd\_rocc\_test**: To test just the vadd\_rocc functionality (unit test),
  we include vadd\_rocc\_test. This test links directly against vadd\_rocc.o to
  minimize dependencies, but external tests should link against libvadd\_rocc.a.

## Building and Testing
You can build the library or test independently or just build them all in one shot:

    $ make all

If all went well, you can run the unit test to make sure the baseline is working:

    $ ./vadd_rocc_test
    ...
    Success!

The default targets build for the current host architecture (likely x86), you
can change this by defining ARCH=riscv before building, e.g.:

    $ make ARCH=riscv

## Using the Generated Accelerator
This repository does not include wrappers for the centrifuge-generated
accelerator. To see that, you'll need to use centrifuge's `generate\_sw`
command which should generate a wrapper elsewhere that has a compatible
interface and can be similarly compiled into a library. This folder was
carefully designed to match the output of `generate_sw` so that switching
between the baseline and accelerated versions is as easy as changing the -L and
-I arguments to the compiler/linker (see the full vadd benchmark for details).
