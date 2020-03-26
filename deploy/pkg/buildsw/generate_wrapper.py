#!/usr/bin/env python3
import argparse
import re
import pathlib
import collections
import logging
from .. import util

# class MmioArg():
#     def __init__(self, name, addr, size=1):
#         self.name = name
#         self.addr = int(addr, 0)
#         self.size = size
#
#     def incrementSize(self):
#         if self.size < 2:
#             self.size += 1
#         else:
#             raise RuntimeError("Invalid size for argument " + self.name + ": " + str(self.size + 1))
#
#     def cType(self):
#         """Returns a string representing the C type of this argument"""
#         if self.size == 0:
#             return "void"
#         elif self.size == 1:
#             return "uint32_t"
#         elif self.size == 2:
#             return "uint64_t"
#         else:
#             raise RuntimeError("Unsupported variable size: " + str(self.size))

def cType(size):
    """Returns a string representing the C type of this argument based on size (assumes uint)"""
    if size == 0:
        return "void"
    elif size == 32:
        return "uint32_t"
    elif size == 64:
        return "uint64_t"
    else:
        raise RuntimeError("Unsupported variable size: " + str(size))

def generateHeader(fname, body):
    """Given the signature of the accelerated function, return an appropriate
    header file. """

    prefix = fname.upper().replace("-", "_") + "_"
    header = ("#ifndef " + prefix + "WRAPPER_H\n"
              "#define " + prefix + "WRAPPER_H\n")

    header += '\n'
    header += body + "\n"
    # header += signature + ";\n"
    header += "#endif"
    return header

def generateSignature(func):
    signature = ""
    if func.retSize == 64:
        retStr = "uint64_t"
    elif func.retSize == 32:
        retStr = "uint32_t"
    elif func.retSize == 0:
        retStr = "void"
    else:
        raise util.ConfigurationError(
            "Invalid return size (" + str(func.retSize) + "b): must be either 0, 32, or 64")

    signature += retStr + " " + func.name + "_cf_accel("
    argStrs = []
    for arg in func.args:
        argStrs.append("uint" + str(arg.size) + "_t " + arg.name)

    signature += ", ".join(argStrs)
    signature += ")"

    return signature

def ident(level):
    """Return a string of spaces representing an indentation to level 'level'"""
    return "    "*level

def cleanRoccArg(body):
    """Cleans up a RoccArg. Returns True if the argument was cleaned and can be
    used, returns False if the argument should be ignored."""

    reIgnore = re.compile('ap_clk.*|ap_rst.*|\S+_req_full_n|\S+_rsp_empty_n')
    reBaseName = re.compile('ap_(\S+)|(\S+)_datain')

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


def parseVerilogTL(vpath, func: util.funcSignature) -> util.funcSignature:
    """Parse a centrifuge-generated verilog file to extract the information
    needed to generate tilelink wrappers.

    This information is added to func as additional fields:
        func.retAddr : MMIO address used to return values
        func.args[N].addr : MMIO address used to pass each argument

    If func already contains these fields, they will be overwritten

    Args:
        vpath: Path to the verilog file containing control signal info (path-like object)
        func: Function signature of the function being accelerated by this verilog file
    
    returns the modified func
    """

    if hasattr(func, 'retAddr'):
        del func.retAddr

    for arg in func.args:
        if hasattr(arg, 'addr'):
            del arg.addr

    with open(vpath, 'r') as vf:
        print("Parsing: ",vpath)
        reStart = re.compile("^//------------------------Address Info------------------")
        reEnd = re.compile("^//------------------------Parameter----------------------")
        reAddr = re.compile("(0x\S+) : Data signal of (\S+)")
        
        # Note that the order matters here, we only take the first listed address of each argument
        inHeader = False
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
                        addr = int(m.group(1), 16)
                        if name == 'ap_return':
                            if not hasattr(func, 'retAddr'):
                                func.retAddr = addr
                        else:
                            for arg in func.args:
                                if arg.name == name:
                                    if not hasattr(arg, 'addr'):
                                        arg.addr = addr
                                    break

        return func

def generateWrapperRocc(func: util.funcSignature, roccIdx, hName):
    """Returns a Rocc C wrapper given a function.
    
    fname - name of the function
    inputs - list of argument names
    retVal - boolean indicating whether or not a value is returned
    """

    # current indentation level
    lvl = 0

    cWrapper = ('#include "rocc.h"\n'
                '\n'
                '#define ' + func.name.upper() + '_WRAPPER\n'
                '#include ' + '"' + hName + '"\n'
                '\n')

    cWrapper += ident(lvl) + "#define XCUSTOM_ACC " + str(roccIdx) + "\n"
    cWrapper += "\n"

    signature = generateSignature(func)

    cWrapper += signature + "\n"
    cWrapper += "{\n"

    lvl = 1
    if func.retSize != 0:
        cWrapper += ident(lvl) + "uint64_t ret_val;\n"
        cWrapper += "\n"

        if len(func.args) == 0:
            cWrapper += ident(lvl) + "ROCC_INSTRUCTION_D(XCUSTOM_ACC, ret_val, 0);\n"
        elif len(func.args) == 1:
            cWrapper += ident(lvl) + "ROCC_INSTRUCTION_DS(XCUSTOM_ACC, ret_val, " + func.args[0].name + ", 0);\n"
        elif len(func.args) == 2:
            cWrapper += ident(lvl) + "ROCC_INSTRUCTION_DSS(XCUSTOM_ACC, ret_val, " + \
                        func.args[0].name + ", " + func.args[1].name + ", 0);\n"
        else:
            raise util.ConfigurationError("Too many inputs. Rocc only supports up to 2 arguments, was passed " + len(func.args))

    else:
        if len(func.args) == 0:
            cWrapper += ident(lvl) + "ROCC_INSTRUCTION(XCUSTOM_ACC, 0);\n"
        elif len(func.args) == 1:
            cWrapper += ident(lvl) + "ROCC_INSTRUCTION_S(XCUSTOM_ACC, " + func.args[0].name + ", 0);\n"
        elif len(func.args) == 2:
            cWrapper += ident(lvl) + "ROCC_INSTRUCTION_SS(XCUSTOM_ACC, " + func.args[0].name + ", 0);\n"
        else:
            raise ValueError("Too many inputs. Rocc only supports up to 2 arguments, was passed " + len(func.args))

    cWrapper += ident(lvl) + "ROCC_BARRIER();\n"

    cWrapper += '\n'
    if func.retSize != 0:
        cWrapper += ident(lvl) + "return (uint" + str(func.retSize) + "_t)ret_val;\n"

    cWrapper += "}"

    return cWrapper, generateHeader(func.name, signature + ";")

def generateWrapperTL(func: util.funcSignature, baseAddr, hName):
    """Given a set of mmio address/function signature, produce the C wrapper headers (returned as a strings).

    func: Signature of function to wrap
    baseAddr: MMIO base address to use
    hName: Name of corresponding header file
    """

    # name of return signal
    retName = 'ap_return'

    # Prefix to append to all macro constants in the wrapper
    def constPrefix(suffix):
        return "CF_" + func.name.upper() + "_" + suffix

    cWrapper = ('#include "mmio.h"\n'
                '#define ' + constPrefix("WRAPPER") + "\n"
                '#include ' + '"' + hName + '"\n'
                '#include "centrifuge.h"'
                '\n'
                '#define AP_DONE_MASK 0b10\n')

    # MMIO Constants
    cWrapper += "#define " + constPrefix("BASE") + " " + str(baseAddr) + "\n"
    cWrapper += "#define " + constPrefix("INT") + " 0x4\n"
    for arg in func.args:
        cWrapper += "#define " + constPrefix(arg.name) + "_0 "+ hex(arg.addr) + "\n"
        if arg.size == 64:
            cWrapper += "#define " + constPrefix(arg.name) + "_1 " + hex(arg.addr + 0x4) + "\n"
    if func.retSize != 0:
        cWrapper += "#define " + constPrefix(retName) + "_0 " + hex(func.retAddr) + "\n"
        if func.retSize == 64:
            cWrapper += "#define " + constPrefix(retName) + "_1 " + hex(func.retAddr + 0x4) + "\n"
    cWrapper += "\n"

    # Create the function signature
    signature = generateSignature(func)
    # signature = ""
    # if func.retSize == 0:
    #     retStr = "void"
    # else:
    #     retStr = retVal.cType()
    #
    # signature += retStr + " " + fname + "_cf_accel("
    # argStrs = []
    # for arg in args:
    #     argStrs.append(arg.cType() + " " + arg.name)
    # signature += ", ".join(argStrs)
    # signature += ")"

    cWrapper += signature + "\n"
    cWrapper += "{\n"

    # Pass Args to MMIO
    cWrapper += "    //Disable Interrupts\n"
    cWrapper += "    reg_write32(" + constPrefix("BASE") + " + " + constPrefix("INT") + ", 0x0);\n"
    for arg in func.args:
        cWrapper += "    reg_write32(" + constPrefix("BASE") + " + " + constPrefix(arg.name) + "_0, (uint32_t) " + arg.name + ");\n"
        if arg.size == 64:
            cWrapper += "    reg_write32(" + constPrefix("BASE") + " + " + constPrefix(arg.name) + "_1, (uint32_t) (" + arg.name + " >> 32));\n"

    # Execute Accelerator
    cWrapper += ("    // Write to ap_start to start the execution \n"
                 "    reg_write32(" + constPrefix("BASE") + ", 0x1);\n"
                 "\n"
                 "    // Done?\n"
                 "    int done = 0;\n"
                 "    while (!done){\n"
                 "        done = reg_read32(" + constPrefix("BASE") + ") & AP_DONE_MASK;\n"
                 "    }\n")

    # Handle returns (if any)
    if func.retSize != 0:
        cWrapper += "\n"
        cWrapper += "    " + cType(func.retSize) + " ret_val = 0;\n"
        cWrapper += "    ret_val = reg_read32(" + constPrefix("BASE") + " + " + constPrefix(retName) + "_0);\n"
        if func.retSize == 64:
            cWrapper += "    ret_val |= reg_read32(" + constPrefix("BASE") + " + " + constPrefix(retName) + "_1) " + \
                        ">> 32;\n"

    cWrapper += "}"

    headBody = ("#define " + constPrefix("BASE") + " " + str(baseAddr) + "\n\n"
            + signature + ";\n")

    return cWrapper, generateHeader(func.name, headBody)

def generateSW(accels):
    """Generate all software wrappers for the specified set of accelerators (util.AccelConfig)"""
    logger = logging.getLogger()

    # Create the directory for generated outputs. We may overwrite things in
    # this directory as-needed.
    accels.gensw_dir.mkdir(exist_ok=True)

    for accel in accels.rocc_accels:
        headerName = accel.name + '_rocc_wrapper.h'
        # inputs, retVal = parseVerilogRocc(accel.verilog_dir / (accel.name + ".v"))
        # cWrapper, hWrapper = generateWrapperRocc(accel.func, accel.rocc_insn_id, inputs, retVal, headerName)
        cWrapper, hWrapper = generateWrapperRocc(accel.func, accel.rocc_insn_id, headerName)
        accel.wrapper_dir = accels.gensw_dir / accel.name
        accel.wrapper_dir.mkdir(exist_ok=True)

        cPath = accel.wrapper_dir / (accel.name + '_rocc_wrapper.c')
        if cPath.exists():
            cPath.unlink()
        with open(cPath , 'w') as cF:
            cF.write(cWrapper)

        hPath = accel.wrapper_dir / headerName
        if hPath.exists():
            hPath.unlink()
        with open(hPath , 'w') as hF:
            hF.write(hWrapper)

    for accel in accels.tl_accels:
        headerName = accel.name + '_tl_wrapper.h'
        # cWrapper, hWrapper = generateWrapperTL(accel.func, accel.base_addr, funcArgs, retVal, headerName)
        accel.func = parseVerilogTL(accel.verilog_dir / (accel.name + "_control_s_axi.v"), accel.func)
        cWrapper, hWrapper = generateWrapperTL(accel.func, accel.base_addr, headerName)
        accel.wrapper_dir = accels.gensw_dir / accel.name
        accel.wrapper_dir.mkdir(exist_ok=True)

        cPath = accel.wrapper_dir / (accel.name + '_tl_wrapper.c')
        if cPath.exists():
            cPath.unlink()
        with open(cPath , 'w') as cF:
            cF.write(cWrapper)

        hPath = accel.wrapper_dir / headerName
        if hPath.exists():
            hPath.unlink()
        with open(hPath , 'w') as hF:
            hF.write(hWrapper)

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
        # funcArgs, retVal = parseVerilogTL(
        #         args.source / 'src' / 'main' / 'verilog' / (args.prefix + args.fname + "_control_s_axi.v"))

        # cWrapper, hWrapper = generateWrapperTL(args.fname, args.base, funcArgs, retVal)
        cWrapper, hWrapper = generateWrapperTL(args.func, args.base)
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

