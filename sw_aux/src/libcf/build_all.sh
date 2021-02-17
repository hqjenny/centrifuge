#!/bin/bash
set -e
make ARCH=riscv HOST=baremetal ACCEL=Y install
make ARCH=riscv HOST=baremetal install
make ARCH=riscv HOST=linux ACCEL=Y install
make ARCH=riscv HOST=linux install
make ARCH=x86 HOST=linux install
