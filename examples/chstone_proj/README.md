# Run adpcm example
Change dir to `chipyard/tools/centrifuge/`
```
 ./deploy/centrifuge -c examples/chstone_proj/src/hls/adpcm/adpcm_soc.json generate_hw
 ./deploy/centrifuge -c examples/chstone_proj/src/hls/adpcm/adpcm_soc.json generate_sw
```

Now the wrapper of software is generated at `examples/chstone_proj/src/hls/adpcm/centrifuge_wrappers/rocc0_adpcm_encode`.
We need to compile the software wrapper to a static library object by running the a make command under the folder:
```
pushd examples/chstone_proj/src/hls/adpcm/centrifuge_wrappers/rocc0_adpcm_encode && make && popd
```

We provided an exmaple to compile the software to call the the accelerator under `examples/chstone_proj/src/c/adpcm/`. You can simply run a make to compile it:  

```
pushd examples/chstone_proj/src/c/adpcm && make && popd
```
The reference software only binary `adpcm.bm.rv` and the accelerated binary is `adpcm.bm_accel.rv`. 
You can run them on VCS and FireSim simulation.  
