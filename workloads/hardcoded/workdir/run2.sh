#!/bin/bash

cd /root/

echo "BASELINE"
echo "PROGRAM: bitonic_sort" 
./bitonic_sort
echo "PROGRAM: adpcm_encode"
./adpcm_encode
echo "PROGRAM: adpcm_decode"
./adpcm_decode

echo "RoCC"
echo "PROGRAM: bitonic_sort" 
./bitonic_sort.inst
echo "PROGRAM: adpcm_encode"
./adpcm_encode.inst
echo "PROGRAM: adpcm_decode"
./adpcm_decode.inst

poweroff
