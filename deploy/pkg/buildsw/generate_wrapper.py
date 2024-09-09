#!/usr/bin/env python3
import argparse
import re
import pathlib
import collections
import logging
from .. import util
import shutil

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

def generateHeader(signature):
    """Given the signature of the accelerated function, return an appropriate
    header file. """

    header = ("#ifndef ACCEL_WRAPPER_H\n"
              "#define ACCEL_WRAPPER_H\n")

    header += '\n'
    header += signature + ";\n"
    header += "#endif"
    return header

def ident(level):
    """Return a string of spaces representing an indentation to level 'level'"""
    return "    "*level

def cleanRoccArg(body):
    """Cleans up a RoccArg. Returns True if the argument was cleaned and can be
    used, returns False if the argument should be ignored."""

    reIgnore = re.compile('ap_clk.*|ap_rst.*|\S+_req_full_n|\S+_rsp_empty_n')
    reBaseName = re.compile('ap_(\S+)|(\S+)_datain|(\S+);')

    if reIgnore.match(body):
        return None

    m = reBaseName.match(body)
    if not m:
        raise ValueError("Could not parse argument name: " + body)

    return m.group(m.lastindex)

def parseVerilogRocc(vpath):
    """Parse a centrifuge-generated verilog file to extract the information
    needed to generate a RoCC wrapper.

    vpath: Path to main verilog function file

    Returns: (inputs, retVal)
        inputs - list of argument names
        retVal - boolean indicating whether or not a return value is present
    """

    # Input/Output statements in the verilog. We assume only one module in the file.
    reInput = re.compile('^\s*input\s+\[.*:.*\]\s*(.*)')
    reReturnVal = re.compile('^\s*output\s+\[(.*):(.*)\]\s*ap_return;')

    print("Parsing: ",vpath)
    inputs = []
    retVal = False
    with open(vpath, 'r') as vf:
        for line in vf.readlines():
            inMatch = reInput.match(line)
            if inMatch:
                argName = cleanRoccArg(inMatch.group(1))
                if argName:
                    inputs.append(argName)
            else:
                if reReturnVal.match(line):
                    retVal = True

        return inputs, retVal


def parseVerilogTL(vpath):
    """Parse a centrifuge-generated verilog file to extract the information
    needed to generate tilelink wrappers.

    vpath: Path to the verilog file containing control signal info (path-like object)

    returns: (returnSize, Args)
        retVal: MmioArg representing the return value (or None if no return).
        Args: List of MmioArg representing the arguments to the accelerated function
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

        return (list(args.values()), retVal)

def generateWrapperRocc(fname, roccIdx, inputs, retVal, fname_header):
    """Returns a Rocc C wrapper given a function.

    fname - name of the function
    inputs - list of argument names
    retVal - boolean indicating whether or not a value is returned
    """

    # current indentation level
    lvl = 0

    cWrapper = ('#include "rocc.h"\n'
                '\n'
                '#define ACCEL_WRAPPER\n'
                f'#include "{fname_header}"\n'
                '\n')

    cWrapper += ident(lvl) + "#define XCUSTOM_ACC " + str(roccIdx) + "\n"
    cWrapper += "\n"

    signature = ""
    if retVal:
        retStr = "uint64_t"
    else:
        retStr = "void"

    signature += retStr + " " + fname + "_wrapper("
    argStrs = []
    for arg in inputs:
        argStrs.append("uint64_t " + arg)
    signature += ", ".join(argStrs)
    signature += ")"
    cWrapper += signature + "\n"
    cWrapper += "{\n"

    lvl = 1
    if retVal:
        cWrapper += ident(lvl) + "uint64_t ret_val;\n"
        cWrapper += "\n"

        if len(inputs) == 0:
            cWrapper += ident(lvl) + "ROCC_INSTRUCTION_D(XCUSTOM_ACC, ret_val, 0);\n"
        elif len(inputs) == 1:
            cWrapper += ident(lvl) + "ROCC_INSTRUCTION_DS(XCUSTOM_ACC, ret_val, " + inputs[0] + ", 0);\n"
        elif len(inputs) == 2:
            cWrapper += ident(lvl) + "ROCC_INSTRUCTION_DSS(XCUSTOM_ACC, ret_val, " + inputs[0] + ", " + inputs[1] + ", 0);\n"
        else:
            raise ValueError("Too many inputs. Rocc only supports up to 2 arguments, was passed " + len(inputs))

    else:
        if len(inputs) == 0:
            cWrapper += ident(lvl) + "ROCC_INSTRUCTION(XCUSTOM_ACC, 0);\n"
        elif len(inputs) == 1:
            cWrapper += ident(lvl) + "ROCC_INSTRUCTION_S(XCUSTOM_ACC, " + inputs[0] + ", 0);\n"
        elif len(inputs) == 2:
            cWrapper += ident(lvl) + "ROCC_INSTRUCTION_SS(XCUSTOM_ACC, " + inputs[0] + ", 0);\n"
        else:
            raise ValueError("Too many inputs. Rocc only supports up to 2 arguments, was passed " + len(inputs))

    cWrapper += ident(lvl) + "ROCC_BARRIER();\n"

    cWrapper += '\n'
    if retVal:
        cWrapper += ident(lvl) + "return ret_val;\n"

    cWrapper += "}"

    return cWrapper, generateHeader(signature)

def generateWrapperTL(fname, baseAddr, args, retVal, fname_header):
    """Given a set of mmio address/varialble pairs, produce the C wrapper
    (returned as a string).

    fname: Name to use for the function
    baseAddr: MMIO base address to use
    retVal: MmioArg representing the return value (may be None)
    args: List of MmioArg representing the function inputs
    """

    cWrapper = ('#include "mmio.h"\n'
                '#define ACCEL_WRAPPER\n'
                f'#include "{fname_header}"\n'
                '\n'
                '#define AP_DONE_MASK 0b10\n')

    # MMIO Constants
    cWrapper += "#define ACCEL_BASE " + str(baseAddr) + "\n"
    cWrapper += "#define ACCEL_INT 0x4\n"
    for arg in args + ([retVal] if retVal is not None else []):
        cWrapper += "#define ACCEL_"+arg.name+"_0 "+ hex(arg.addr) + "\n"
        if arg.size == 2:
            cWrapper += "#define ACCEL_"+arg.name+"_1 " + hex(arg.addr + 0x4) + "\n"
    cWrapper += "\n"

    # Create the function signature
    signature = ""
    if retVal is None:
        retStr = "void"
    else:
        retStr = retVal.cType()

    signature += retStr + " " + fname + "_wrapper("
    argStrs = []
    for arg in args:
        argStrs.append(arg.cType() + " " + arg.name)
    signature += ", ".join(argStrs)
    signature += ")"

    cWrapper += signature + "\n"
    cWrapper += "{\n"

    # Pass Args to MMIO
    cWrapper += "    //Disable Interrupts\n"
    cWrapper += "    reg_write32(ACCEL_BASE + ACCEL_INT, 0x0);\n"
    for arg in args:
        cWrapper += "    reg_write32(ACCEL_BASE + ACCEL_"+arg.name+"_0, (uint32_t) "+arg.name+");\n"
        if arg.size == 2:
            cWrapper += "    reg_write32(ACCEL_BASE + ACCEL_"+arg.name+"_1, (uint32_t) ("+arg.name+" >> 32));\n"

    # Execute Accelerator
    cWrapper += ("    // Write to ap_start to start the execution \n"
                 "    reg_write32(ACCEL_BASE, 0x1);\n"
                 "\n"
                 "    // Done?\n"
                 "    int done = 0;\n"
                 "    while (!done){\n"
                 "        done = reg_read32(ACCEL_BASE) & AP_DONE_MASK;\n"
                 "    }\n")

    # Handle returns (if any)
    if retVal is not None:
        cWrapper += "\n"
        cWrapper += "    " + retVal.cType() + " ret_val = 0;\n"
        cWrapper += "    ret_val = reg_read32(ACCEL_BASE + ACCEL_"+retVal.name+"_0);\n"
        if retVal.size == 2:
            cWrapper += "    ret_val |= reg_read32(ACCEL_BASE + ACCEL_"+retVal.name+"_1) >> 32;\n"

    cWrapper += "}"

    return cWrapper, generateHeader(signature)

def generateSW(accels):
    """Generate all software wrappers for the specified set of accelerators (util.AccelConfig)"""
    logger = logging.getLogger()

    # Create the directory for generated outputs. We may overwrite things in
    # this directory as-needed.
    accels.gensw_dir.mkdir(exist_ok=True)

    template_dir = util.getOpt('template-dir')
    static_lib_makefile = template_dir / 'wrapper_makefile_template'

    logger.info('Generate software wrappers...')
    for accel in accels.rocc_accels:
        inputs, retVal = parseVerilogRocc(accel.verilog_dir / (accel.name + ".v"))
        fname_header = accel.name + '_rocc_wrapper.h'
        cWrapper, hWrapper = generateWrapperRocc(accel.func, accel.rocc_insn_id, inputs, retVal, fname_header)
        accel.wrapper_dir = accels.gensw_dir / accel.name
        accel.wrapper_dir.mkdir(exist_ok=True)
        shutil.copy(static_lib_makefile, accel.wrapper_dir / 'Makefile')

        cPath = accel.wrapper_dir / (accel.name + '_rocc_wrapper.c')
        if cPath.exists():
            cPath.unlink()
        with open(cPath , 'w') as cF:
            cF.write(cWrapper)
        hPath = accel.wrapper_dir / (accel.name + '_rocc_wrapper.h')
        if hPath.exists():
            hPath.unlink()
        with open(hPath , 'w') as hF:
            hF.write(hWrapper)
        logger.info('RoCC accelerator path: {} {}'.format(cPath, hPath))

    for accel in accels.tl_accels:
        funcArgs, retVal = parseVerilogTL(accel.verilog_dir / (accel.name + "_control_s_axi.v"))
        fname_header = accel.name + '_tl_wrapper.h'
        cWrapper, hWrapper = generateWrapperTL(accel.func, accel.base_addr, funcArgs, retVal, fname_header)
        accel.wrapper_dir = accels.gensw_dir / accel.name
        accel.wrapper_dir.mkdir(exist_ok=True)
        shutil.copy(static_lib_makefile, accel.wrapper_dir / 'Makefile')

        cPath = accel.wrapper_dir / (accel.name + '_tl_wrapper.c')
        if cPath.exists():
            cPath.unlink()
        with open(cPath , 'w') as cF:
            cF.write(cWrapper)

        hPath = accel.wrapper_dir / (accel.name + '_tl_wrapper.h')
        if hPath.exists():
            hPath.unlink()
        with open(hPath , 'w') as hF:
            hF.write(hWrapper)
        logger.info('TL accelerator path: {} {}'.format(cPath, hPath))

    # Creates the Makefile for the last accel in the json if there is more than one
    with open(accels.accel_json_dir / 'Makefile', 'w') as f:
        makefile_vars = {
            'VERBOSE': '1',
            'TARGET': accel.pgm,
            'FUNC': accel.func,
            'LDFLAGS': '',
            'CFLAGS': '',
            'SRC': ' '.join(str(src) for src in accel.srcs),
        }
        for var, val in makefile_vars.items():
            print(var + '=' + val, file=f)

        s = """
ifeq ($(CUSTOM_INST), 1)
	CFLAGS+=-DCUSTOM_INST
endif

ifeq ($(CUSTOM_DRIVER), 1)
	CFLAGS+=-DCUSTOM_DRIVER
endif

ifeq ($(LLVM), 1)
	ACCEL ?=0
	include ../Makefile.llvm.in
else
ifeq ($(GCC), 1)
	include ../Makefile.gcc.in
else
	include ../Makefile.bm.in
endif
endif"""
        print(s, file=f)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
            description="Generate software wrappers for a given centrifuge-generated function.")

    parser.add_argument('-n', '--fname', required=True, help="Name of function to accelerate")
    parser.add_argument('-b', '--base', required=True, help="Base address of function (if tilelink), RoCC index (if rocc)")
    parser.add_argument('-p', '--prefix', default="", help="Optional prefix for function")
    parser.add_argument('-m', '--mode', required=True,
            help="Function integration mode (either 'tl' or 'rocc')")
    parser.add_argument('-s', '--source', required=True, type=pathlib.Path,
            help="Path to the source directory to use when generating (e.g. 'centrifuge/accel/hls_example_func/').")

    args = parser.parse_args()

    if args.mode == 'tl':
        funcArgs, retVal = parseVerilogTL(
                args.source / 'src' / 'main' / 'verilog' / (args.prefix + args.fname + "_control_s_axi.v"))

        cWrapper, hWrapper = generateWrapperTL(args.fname, args.base, funcArgs, retVal)
    elif args.mode == 'rocc':
        inputs, retVal = parseVerilogRocc(
                args.source / 'src' / 'main' / 'verilog' / (args.prefix + args.fname + ".v"))
        cWrapper, hWrapper = generateWrapperRocc(args.fname, args.base, inputs, retVal)
    else:
        raise NotImplementedError("Mode '" + args.mode + "' not supported.")


    with open(args.source / 'src' / 'main' / 'c' / 'accel_wrapper.c', 'w') as cF:
        cF.write(cWrapper)

    with open(args.source / 'src' / 'main' / 'c' / 'accel_wrapper.h', 'w') as hF:
        hF.write(hWrapper)

