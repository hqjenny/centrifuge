#!/bin/bash

cd /root/

echo "BASELINE"
echo "PROGRAM: dot_product_dot" 
./dot_product_dot
echo "PROGRAM: gsm_lpc"
./gsm_lpc
echo "PROGRAM: sha_sha_update"
./sha_sha_update

echo "RoCC"
echo "PROGRAM: dot_product_dot" 
./dot_product_dot.inst
echo "PROGRAM: gsm_lpc"
./gsm_lpc.inst
echo "PROGRAM: sha_sha_update"
./sha_sha_update.inst

poweroff
