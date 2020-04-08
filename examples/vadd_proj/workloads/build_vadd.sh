#!/bin/bash
set -e

make -C ../ ARCH=riscv HOST=linux bench
make -C ../ ARCH=riscv HOST=bare bench
