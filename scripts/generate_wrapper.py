#!/usr/bin/env python3
import argparse
import re
import pathlib
import collections

class MmioArg():
    def __init__(self, name, addr, size=1):
        self.name = name
        self.addr = int(addr, 0)
        self.size = size

    def incrementSize(self):
        if self.size < 2:
            self.size += 1
        else:
            raise RuntimeError("Invalid size for argument " + self.name + ": " + str(self.size + 1))

    def cType(self):
        """Returns a string representing the C type of this argument"""
        if self.size == 0:
            return "void"
        elif self.size == 1:
            return "uint32_t"
        elif self.size == 2:
            return "uint64_t"
        else:
            raise RuntimeError("Unsupported variable size: " + str(self.size))

def parseVerilogTL(vpath):
    """Parse a centrifuge-generated verilog file to extract the information
    needed to generate tilelink wrappers.
    
    vpath: Path to the verilog file to parse (path-like object)
    
    returns: (returnSize, Args)
        returnSize: The size of the return value in 32-bit words. Either 0, 1, or 2.
        Args: An ordered dictionary of mappings from argument name to base address
    """
    with open(vpath, 'r') as vf:
        print("Parsing: ",vpath)
        reStart = re.compile("^//------------------------Address Info------------------")
        reEnd = re.compile("^//------------------------Parameter----------------------")
        reAddr = re.compile("(0x\S+) : Data signal of (\S+)")
        
        inHeader = False
        args = collections.OrderedDict()
        retVal = None
        for line in vf.readlines():
            if not inHeader:
                if reStart.match(line):
                    inHeader = True
            else:
                if reEnd.match(line):
                    break
                else:
                    m = reAddr.search(line)
                    if m:
                        name = m.group(2)
                        addr = m.group(1)
                        if name == 'ap_return':
                            if retVal is None:
                                retVal = MmioArg(name, addr)
                            else:
                                retVal.incrementSize()
                        elif name in args:
                            args[name].incrementSize()
                        else:
                            args[name] = MmioArg(name, addr)

        return (retVal, list(args.values()))


def generateWrapperTL(fname, baseAddr, retVal, args, cDir):
    """Given a set of mmio address/varialble pairs, produce a corresponding .c and .h file in cDir.

    fname: Name to use for the function
    baseAddr: MMIO base address to use
    retVal: MmioArg representing the return value (may be None)
    args: List of MmioArg representing the function inputs
    cDir: Path to c source directory (pathlike object)
    """

    cWrapper = """
#include "mmio.h"
#include "accel.h"

#define ACCEL_WRAPPER
#define AP_DONE_MASK 0b10

"""
    # MMIO Constants
    cWrapper += "#define ACCEL_BASE " + str(baseAddr) + "\n"
    cWrapper += "#define ACCEL_INT 0x4\n"
    for arg in args + ([retVal] if retVal is not None else []):
        cWrapper += "#define ACCEL_"+arg.name+"_0 "+ hex(arg.addr) + "\n"
        if arg.size == 2:
            cWrapper += "#define ACCEL_"+arg.name+"_1 " + hex(arg.addr + 0x4) + "\n"
    cWrapper += "\n"

    # Create the function signature
    if retVal is None:
        retStr = "void"
    else:
        retStr = retVal.cType()

    cWrapper += retStr + " " + fname + "("
    argStrs = []
    for arg in args:
        argStrs.append(arg.cType() + " " + arg.name)
    cWrapper += ", ".join(argStrs)
    cWrapper += ")\n"
    cWrapper += "{\n"

    # Pass Args to MMIO
    cWrapper += "    //Disable Interrupts\n"
    cWrapper += "    reg_write32(ACCEL_BASE + ACCEL_INT, 0x0)\n"
    for arg in args:
        cWrapper += "    reg_write32(ACCEL_BASE + ACCEL_"+arg.name+"_0, (uint32_t) "+arg.name+");\n"
        if arg.size == 2:
            cWrapper += "    reg_write32(ACCEL_BASE + ACCEL_"+arg.name+"_1, (uint32_t) ("+arg.name+" >> 32));\n"

    # Execute Accelerator
    cWrapper +="""
    // Write to ap_start to start the execution 
    reg_write32(ACCEL_BASE, 0x1);

    // Done?
    int done = 0;
    while (!done){
        done = reg_read32(ACCEL_BASE) & AP_DONE_MASK;
    }

"""

    # Handle returns (if any)
    if retVal is not None:
        cWrapper += "    " + retVal.cType() + " ret_val = 0;\n"
        cWrapper += "    ret_val = reg_read32(ACCEL_BASE + ACCEL_"+retVal.name+"_0);\n"
        if retVal.size == 2:
            cWrapper += "    ret_val |= reg_read32(ACCEL_BASE + ACCEL_"+retVal.name+"_1) >> 32;\n"

    cWrapper += "}"

    return cWrapper

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
            description="Generate software wrappers for a given centrifuge-generated function.")

    parser.add_argument('-n', '--fname', required=True, help="Name of function to accelerate")
    parser.add_argument('-b', '--base', required=True, help="Base address of function (if tilelink)")
    parser.add_argument('-p', '--prefix', default="", help="Optional prefix for function")
    parser.add_argument('-m', '--mode', required=True,
            help="Function integration mode (either 'tl' or 'rocc')")
    parser.add_argument('-s', '--source', required=True, type=pathlib.Path,
            help="Path to the source directory to use when generating (e.g. 'centrifuge/accel/hls_example_func/').")

    args = parser.parse_args()

    if args.mode == 'tl':
        retVal, funcArgs = parseVerilogTL(
                args.source / 'src' / 'main' / 'verilog' / (args.prefix + args.fname + "_control_s_axi.v"))
        print(generateWrapperTL(args.fname, args.base, retVal, funcArgs, "."))
