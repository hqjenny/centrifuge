#!/bin/bash
set -e

pushd ../

pushd src/hls/vadd_tl
make ARCH=riscv
popd

pushd src/hls/vadd_rocc
make ARCH=riscv
popd

pushd src/benchmark
make ARCH=riscv
popd

popd
