#!/usr/bin/env python3
import os
import argparse
import json
import subprocess as sp
import shutil
import sys

binDir = "./workdir/bin"

def buildBench(fullName, pDir, pName, env):
    if not os.path.exists(pDir):
        print("Building benchmark " + fullName + " failed: ")
        print("\t" + pDir + ": Path does not exist")
        return

    try:
        # The makefiles are a bit wonky because they depend on an environment
        # variable which means they don't rebuild when you change between
        # ROCC/TL/Baseline. We just always rebuild.
        sp.call("make cleanall", cwd=pDir, shell=True)
        out = sp.check_output("make", shell=True, cwd=pDir, env=env, stderr=sp.STDOUT, universal_newlines=True)
    except Exception as e:
        print("Building benchmark " + fullName + " failed:")
        print(e.output)
        return

    shutil.copy2(os.path.join(pDir, pName), os.path.join(binDir, fullName))

def main():
    parser = argparse.ArgumentParser(
            description="Build system for linux-based benchmarks")

    parser.add_argument('-r', '--rocc', action='store_true', help='Build ROCC versions')
    parser.add_argument('-t', '--tilelink', action='store_true', help='Build tilelink versions')
    parser.add_argument('-b', '--baseline', action='store_true', help='Build baseline versions')
    parser.add_argument('-f', '--file', default="all.json", help='Provide a list of benchmarks to build (JSON of the form { "PROG" : [ "func0", "func1"], ...}). Without this, all benchmarks will be built.')

    args = parser.parse_args()

    with open(args.file, 'r') as jFile:
        bms = json.load(jFile)

    # if not os.path.exists(binDir):
    os.makedirs(binDir, exist_ok=True)

    origEnv = os.environ.copy()
    roccEnv = dict(origEnv, **{ "CUSTOM_INST" : "1" })
    TLEnv = dict(origEnv, **{ "CUSTOM_DRIVER" : "1" })

    for prog,fList in bms.items():
        for func in fList:
            if args.rocc:
                dstName = prog + "_" + func + ".inst"
                pDir = prog + "_" + func 
                pName = prog + ".inst"
                buildBench(dstName, pDir, pName, roccEnv)

            if args.tilelink:
                dstName = "_".join((prog, "tl", func + ".driver"))
                pDir = "_".join((prog, "tl", func))
                pName = prog + "_tl" + ".driver"
                buildBench(dstName, pDir, pName, TLEnv)

            if args.baseline:
                dstName = "_".join((prog, func))
                pDir = "_".join((prog, func))
                pName = prog
                buildBench(dstName, pDir, pName, origEnv)

main()
