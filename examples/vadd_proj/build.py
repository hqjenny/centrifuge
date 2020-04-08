#!/usr/bin/env python3
import pathlib
import subprocess as sp
import shutil
import argparse

"""Build the various libraries and benchmarks for the example vadd project."""

buildDir = pathlib.Path("./build")
srcDirs = {
        "tl_base" : pathlib.Path("src/hls/vadd_tl"),
        "rocc_base" : pathlib.Path("src/hls/vadd_rocc"),
        "tl_wrapper" : pathlib.Path("centrifuge_wrappers/tl0_vadd_tl_vadd"),
        "rocc_wrapper" : pathlib.Path("centrifuge_wrappers/rocc0_vadd_rocc_vadd_rocc"),
        "benchmark" : pathlib.Path("src/benchmark")
        }

outputs = {
    "tl_base" : srcDirs['tl_base'] / 'libvadd_tl.a',
    "rocc_base" : srcDirs['rocc_base'] / 'libvadd_rocc.a',
    "tl_wrapper" : srcDirs['tl_wrapper'] / 'libvadd_tl.a',
    "rocc_wrapper" : srcDirs['rocc_wrapper'] / 'libvadd_rocc.a',
    "benchmark" : srcDirs['benchmark'] / 'vadd'
    }

installs = { key : (buildDir / path.name) for key, path in outputs.items() }


class buildOpts:
    def __init__(self, arch='x86', host='linux', accel=False):
        self.arch = arch
        self.host = host
        self.accel = accel


def make(opts: buildOpts, srcDir, target):
    """equivalent to 'make -C srcDir target' with the appropriate opts defined"""
    optStrs = [
            'ARCH=' + opts.arch,
            'HOST=' + opts.host,
            'ACCEL=' + ('1' if opts.accel else '')
    ]

    baseCmd = ['make', '-C', str(srcDir)] + optStrs
    sp.run(baseCmd + ['clean'], check=True)
    sp.run(baseCmd + [str(target)], check=True)


def install(opts: buildOpts, objPath):
    """Install the output at objPath to the build directory""" 
    accelStr = '_accel' if opts.accel else ''
    configStr = "_{}_{}{}".format(opts.arch, opts.host, accelStr)

    dstPath = buildDir / (objPath.stem + configStr + objPath.suffix) 
    shutil.copy(objPath, dstPath)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
            description="Build the example vadd project for different architectures and configurations.")
    parser.add_argument('--arch', '-a', help="Architecture to target.", choices=['x86', 'riscv'], default='x86')
    parser.add_argument('--os', '-s', help="Which host configuration to build for.", choices=['linux', 'baremetal'], default='linux')
    parser.add_argument('--accel', '-x', help="Build the accelerated version.", action='store_true')
    parser.add_argument('--clean', help="Remove previous build artifacts before running the build.", action='store_true')
    args = parser.parse_args()

    if ((args.arch == 'x86' and args.os == 'baremetal') or  
            (args.arch == 'x86' and args.accel)):
           print("Invalid configuration: x86 only supports non-accelerated linux builds.")
           exit(1)

    if args.clean:
        if buildDir.exists():
            shutil.rmtree(buildDir)
    buildDir.mkdir(exist_ok=True)

    opts = buildOpts(
            arch = args.arch,
            host = args.os,
            accel = args.accel
          )

    if args.accel:
        targets = outputs.copy()
        targets.pop('tl_base')
        targets.pop('rocc_base')
    else:
        targets = outputs.copy()
        targets.pop('tl_wrapper')
        targets.pop('rocc_wrapper')

    for target in targets:
        make(opts, srcDirs[target], outputs[target].name)
        install(opts, outputs[target])
