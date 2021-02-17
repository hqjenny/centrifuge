#!/bin/bash
pushd sw_aux/src/libcf/
./build_all.sh
popd

pushd sw_aux/src/baremetal
make install
popd
