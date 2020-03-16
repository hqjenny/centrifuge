#from generate_wrapper import parse_verilog_rocc, parse_verilog_tl 
import pathlib
import shutil
import collections
import logging
import argparse
from string import Template
import re
from .. import util

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG) 

def parse_verilog_input_info(inputs):
    arg_count = 0
    rePointerData = re.compile(r"(\S+)_datain")
    rePointerSignals = re.compile(r"(\S+)_req_full_n|(\S+)_rsp_empty_n|(\S+)_datain")
    reApSignals = re.compile(r"ap_\S+")
    input_info = collections.OrderedDict()
    for k, v in inputs.items():
        # If matched for pointer input
        matchPointerData = rePointerData.match(k)
        matchPointerSignals = rePointerSignals.match(k) 
        matchApSignals = reApSignals.match(k)

        arg_type = None
        if matchPointerData:
            arg_name = matchPointerData.group(1)
            arg_type = 'pointer'
        if not matchPointerSignals and not matchApSignals:
            arg_name = k 
            arg_type = 'scalar'
        if arg_type is not None: 
            input_info[arg_name] = {
                'arg_idx': arg_count, 
                'type': arg_type,
                'width': v['width']
            }  
            arg_count += 1

    # Check for correctness
    if len(input_info) > 2: 
        logger.critical("Only accept function with no more than 2 arguments!")
        raise

    for k, v in inputs.items():
        matchPointerSignals = rePointerSignals.match(k)
        if matchPointerSignals: 
            arg_name = None
            if matchPointerSignals.group(1) is not None:
                arg_name = matchPointerSignals.group(1)
            elif matchPointerSignals.group(2) is not None:
                arg_name = matchPointerSignals.group(2)
            elif matchPointerSignals.group(3) is not None:
                arg_name = matchPointerSignals.group(3)
            else:
                logger.critical("Unexpected Signal {}!".format(k))
                raise

            if arg_name in list(input_info.keys()):
                if 'num_signal' in list(input_info[arg_name].keys()):
                    input_info[arg_name]['num_signal'] += 1
                else:
                    input_info[arg_name]['num_signal'] = 1
            else:
                # arg should be created 
                logger.critical("Unexpected Signal {}!".format(k))
                raise
    for k, v in input_info.items(): 
        if v['type'] == 'pointer':
            if v['num_signal'] != 3: 
                logger.critical("The AP bus interfance did not generate expected number of inputs!")
                raise
        elif v['type'] == 'scalar':
            if v['num_signal'] != 1: 
                logger.critical("The AP bus interfance did not generate expected number of inputs!")
                raise
    return input_info


def generate_rocc_input_info(input_info):
    scalar_data_width = [] 
    scalar_idx = []
    ptr_addr_width = []
    ptr_data_width = []
    ptr_idx = []
    
    for k, v in input_info.items(): 
        if v['type'] == 'scalar':
            scalar_data_width.append(str(v['width']))
            scalar_idx.append(str(v['arg_idx']))
        elif v['type'] == 'pointer':
            ptr_addr_width.append(str(64))
            ptr_data_width.append(str(v['width']))
            ptr_idx.append(str(v['arg_idx']))

    scalar_data_width_arr = ','.join(scalar_data_width)
    scalar_idx_arr = ','.join(scalar_idx)
    ptr_addr_width_arr = ','.join(ptr_addr_width)
    ptr_data_width_arr = ','.join(ptr_data_width)
    ptr_idx_arr = ','.join(ptr_idx)
    
    info_dict = {
        'SCALAR_DATA_WIDTH_ARR': scalar_data_width_arr,
        'SCALAR_IDX_ARR': scalar_idx_arr, 
        'PTR_ADDR_WIDTH_ARR': ptr_addr_width_arr, 
        'PTR_DATA_WIDTH_ARR': ptr_data_width_arr,
        'PTR_IDX_ARR': ptr_idx_arr,
    }
    return info_dict


def get_rocc_scalarIO_count(input_info):
    num_scalar = 0
    for k, v in input_info.items(): 
        if v['type'] == 'scalar':
            num_scalar += 1
    return num_scalar


def generate_rocc_scalarIO(num_scalar):
    if num_scalar > 0: 
        return "\tval scalar_io = HeterogeneousBag(scalar_io_dataWidths.map(w => Input(UInt(w))))\n"
    else:
        return ""


def generate_rocc_scalarIO_stmt0(input_info):
    ret_str = ""
    scalar_idx = 0 
    for k, v in input_info.items(): 
        if v['type'] == 'scalar':
            arg_idx = v['arg_idx'] 
            ret_str += "accel.io.scalar_io({}) := rs{}\n".format(scalar_idx, arg_idx+1)
    return ret_str


def generate_rocc_scalarIO_stmt1(num_scalar):
    scalar_str =  """//Scalar values
for(i <- 0 until accel.io.scalar_io.length){
accel.io.scalar_io(i) := cArgs(accel.scalar_io_argLoc(i))
}"""

    if num_scalar > 0:
        return scalar_str
    else:
        return ""


def generate_rocc_ap_return_stmt(outputs):
    if 'ap_return' in list(outputs.keys()): 
        return "val ap_return = accel.io.ap.rtn\n"
    else:
        return "val ap_return = UInt(4)\n"


def generate_vals(io, width):
    if width == 1:
        val = "{}(Bool())".format(io)
    else: 
        val = "{}(UInt({}))".format(io, width) 
    return val


def generate_args(inputs, outputs):
    reClk = re.compile('ap_clk(.*)')
    #reRst = re.compile('ap_rst(.*)')
    #rstMatch = reRst.match(k)
    args_arr = []
    
    # Inputs
    for k, v in inputs.items():
        clkMatch = reClk.match(k) 
        val = None
        if clkMatch:
            val = "Input(Clock())"
        else:
            val = generate_vals('Input', v['width'])
        arg_str = "val {} = {}".format(k, val)
        args_arr.append(arg_str)
   # Outputs 
    for k, v in outputs.items():
        val = generate_vals('Output', v['width'])
        arg_str = "val {} = {}".format(k, val)
        args_arr.append(arg_str)

    return args_arr


def generate_rocc_opt_ap_signals(inputs, outputs):
    ret_str = ""
    if 'ap_return' in list(outputs.keys()): 
        ret_str += "\tio.ap.rtn := bb.io.ap_return\n"
    if 'ap_rst' in list(inputs.keys()):
        ret_str += "\tbb.io.ap_rst := reset\n"
    if 'ap_clk' in list(inputs.keys()):
        ret_str += "\tbb.io.ap_clk := clock\n"
    return ret_str
    

def generate_rocc_assignment(input_info):
    scalar_template = Template("\tbb.io.${ARG} := io.scalar_io($IDX)\n")
    ptr_template = Template("""\tio.ap_bus($IDX).req.din := bb.io.${ARG}_req_din 
\tbb.io.${ARG}_req_full_n := io.ap_bus($IDX).req_full_n 
\tio.ap_bus($IDX).req_write := bb.io.${ARG}_req_write
\tbb.io.${ARG}_rsp_empty_n := io.ap_bus($IDX).rsp_empty_n
\tio.ap_bus($IDX).rsp_read := bb.io.${ARG}_rsp_read
\tio.ap_bus($IDX).req.address := bb.io.${ARG}_address
\tbb.io.${ARG}_datain := io.ap_bus($IDX).rsp.datain
\tio.ap_bus($IDX).req.dataout := bb.io.${ARG}_dataout
\tio.ap_bus($IDX).req.size := bb.io.${ARG}_size
""")

    ret_str = ""
    rocc_scalar_idx = 0
    rocc_ptr_idx = 0
    for k, v in input_info.items(): 
        if v['type'] == 'scalar':
            d = {'ARG': k, 'IDX': rocc_scalar_idx}
            scalar_str = scalar_template.substitute(d) 
            ret_str += scalar_str
            rocc_scalar_idx += 1
        elif v['type'] == 'pointer':
            d = {'ARG': k, 'IDX': rocc_ptr_idx}
            ptr_str = ptr_template.substitute(d)
            ret_str += ptr_str
            rocc_ptr_idx += 1
    return ret_str


def parse_verilog_arg_line(line, reArg, args):
    """ Parse a Verilog line for args.
    line: Input line string
    reArg: regex for the arg
    args: dict = { arg name: {'width': arg bitwidth}}
    """
    ret = False
    reArgWidth = re.compile('\[(.*):(.*)\]\s*(.*)')
    argMatch = reArg.match(line)
    if argMatch: 
        argName = argMatch.group(1)
        if argName: 
            argWidthMatch = reArgWidth.match(argName)
            end = 0
            start = 0
            if argWidthMatch: 
                end = argWidthMatch.group(1)
                endMatch = re.match(r"(\S+) - 1", end)
                if endMatch:
                    size = endMatch.group(1)
                    end = size - 1
                start = argWidthMatch.group(2) 
                argName = argWidthMatch.group(3)
            width = int(end) - int(start) + 1
            args[argName] = {'width': width} 
            ret = True
    return ret


def parse_verilog_rocc(vpath):
    """ Parse a centrifuge-generated verilog file to extract the information
    needed to generate a RoCC wrapper.

    vpath: Path to main verilog function file

    Returns: (inputs, retVal)
        inputs - dict of argument names and the data width
        retVal - boolean indicating whether or not a return value is present
    """


    # Input/Output statements in the verilog. We assume only one module in the file.
    reInput = re.compile('^\s*input\s+(.*)\s*;')
    reOutput = re.compile('^\s*output\s+(.*)\s*;')
    reReturnVal = re.compile('^\s*output\s+\[(.*):(.*)\]\s*ap_return;')

    inputs = collections.OrderedDict()
    outputs = collections.OrderedDict()

    logger.info("Parsing: {}".format(vpath))
    with open(vpath, 'r') as vf:
        for line in vf.readlines():
            # test if it is output
            match = parse_verilog_arg_line(line, reInput, inputs)
            if not match: 
                # test if it is output
                match = parse_verilog_arg_line(line, reOutput, outputs)

    return (inputs, outputs)


def generate_chisel_rocc(func, idx, inputs, outputs, scala_dir, template_dir):

    ##########################################################
    logger.info("Generating BlackBox file ...") 
    template_name = 'chisel_rocc_blackbox_scala_template'
    with open (template_dir / template_name, 'r') as f:
        template_str= f.read()
    
    t = Template(template_str)
    # Generate arguments
    args_arr = generate_args(inputs, outputs)
    args_str = "\n        ".join(args_arr)

    # Generate spec for args
    input_info = parse_verilog_input_info(inputs) 
    info_dict = generate_rocc_input_info(input_info) 
    return_width = outputs['ap_return']['width'] if 'ap_return' in list(outputs.keys()) else 1
    num_scalar = get_rocc_scalarIO_count(input_info)
    scalar_io_str = generate_rocc_scalarIO(num_scalar)

    # Generate signal assignments
    # For optional vivado ap signals
    ap_return_rst_clk_str = generate_rocc_opt_ap_signals(inputs, outputs)
    signal_assignment_str = generate_rocc_assignment(input_info) 
    
    chisel_dict = {
        "FUNC": func,
        "ARGS": args_str,
        'SCALAR_DATA_WIDTH_ARR': info_dict['SCALAR_DATA_WIDTH_ARR'],
        'SCALAR_IDX_ARR': info_dict['SCALAR_IDX_ARR'], 
        'PTR_ADDR_WIDTH_ARR': info_dict['PTR_ADDR_WIDTH_ARR'], 
        'PTR_DATA_WIDTH_ARR': info_dict['PTR_DATA_WIDTH_ARR'],
        'PTR_IDX_ARR': info_dict['PTR_IDX_ARR'],
        'RETURN_WIDTH': outputs['ap_return']['width'],
        'SCALAR_IO': scalar_io_str,
        'AP_RETURN_RST_CLK': ap_return_rst_clk_str, 
        'SIGNAL_ASSIGNMENT': signal_assignment_str, 

    }

    rocc_blackbox_scala_str = t.substitute(chisel_dict)
    scala_path = scala_dir / pathlib.Path(func + '_blackbox.scala')
    
    logger.info("\t\tGenerate rocc_blackbox code in CHISEL: {}".format(scala_path))
    with open(scala_path,'w') as f:
        f.write(rocc_blackbox_scala_str)


    ##########################################################
    logger.info("Generating Control file ...") 
    template_name = 'chisel_rocc_accel_scala_template'
    with open (template_dir / template_name, 'r') as f:
        template_str= f.read()
    
    t = Template(template_str)
    scalar_io_assignment0 = generate_rocc_scalarIO_stmt0(input_info)
    ap_return_assignment = generate_rocc_ap_return_stmt(outputs)
    num_scalar = get_rocc_scalarIO_count(input_info)
    scalar_io_assignment1 = generate_rocc_scalarIO_stmt1(num_scalar)
    chisel_dict = {
        "FUNC": func,
        'SCALAR_IO_ASSIGNMENT0': scalar_io_assignment0,
        'AP_RETURN_ASSIGNMENT': ap_return_assignment,
        'SCALAR_IO_ASSIGNMENT1': scalar_io_assignment1, 
        }

    rocc_accel_scala_str = t.substitute(chisel_dict)
    scala_path = scala_dir / pathlib.Path(func + '_accel.scala')

    logger.info("\t\tGenerate rocc_accel code in CHISEL: {}".format(scala_path))
    with open(scala_path,'w') as f:
        f.write(rocc_accel_scala_str)

    ##########################################################
    logger.info("Copying Vivado HLS Interface file ...");
    src_path = template_dir / 'ap_bus_scala_template' 
    dst_path = scala_dir / 'ap_bus.scala' 
    shutil.copy(str(src_path), str(dst_path))

    ##########################################################
    logger.info("Copying ROCC  Memory Controller file ...");
    src_path = template_dir / 'memControllerComponents_scala_template' 
    dst_path = scala_dir / 'memControllerComponents.scala' 
    shutil.copy(str(src_path), str(dst_path))

    ##########################################################
    logger.info("Copying Controller Utilities file ...");
    src_path = template_dir / 'controlUtils_scala_template' 
    dst_path = scala_dir / 'controlUtils.scala' 
    shutil.copy(str(src_path), str(dst_path))


def parse_verilog_tl(vpath):
    """Parse a centrifuge-generated verilog file to extract the information
    needed to generate tilelink wrappers.
    
    vpath: Path to the verilog file containing control signal info (path-like object)
    
    returns: (returnSize, Args)
        retVal: MmioArg representing the return value (or None if no return).
        Args: List of MmioArg representing the arguments to the accelerated function
    """
    reInput = re.compile('^\s*input\s+(.*)\s*;')
    reOutput = re.compile('^\s*output\s+(.*)\s*;')
    reParam = re.compile("parameter\s+(C_\S+) =\s+(.*);")

    inputs = collections.OrderedDict()
    outputs = collections.OrderedDict()
    params = collections.OrderedDict()
    buses = collections.OrderedDict()

    logger.info("Parsing: ",vpath)
    with open(vpath, 'r') as vf:
        for line in vf.readlines():
            # test if it is input
            match = parse_verilog_arg_line(line, reInput, inputs)
            if not match:
                # test if it is output
                match = parse_verilog_arg_line(line, reOutput, outputs)
                if not match: 
                    paramMatch = reParam.match(line)
                    if paramMatch: 
                        param  = paramMatch.group(1)
                        width = paramMatch.group(2)
                        params[param] = {'width': width}
                        busMatch = re.match(r"C_M_AXI_(\S+)_DATA_WIDTH", param)
                        if busMatch:
                            bus = busMatch.group(1).lower()
                            buses[bus] = {'width': width}
    return (inputs, outputs, params, buses)

def run_chisel(accel_conf):
    template_dir = util.getOpt('template-dir')
    for accel in accel_conf.rocc_accels:
        logger.info("\tRun CHISEL generation for {}:".format(accel.name))
        inputs, outputs = parse_verilog_rocc(
                accel.verilog_dir / (accel.name + ".v"))
        chiselWrapper = generate_chisel_rocc( accel.name, accel.rocc_insn_id, inputs, outputs, accel.scala_dir, template_dir)

    for accel in accel_conf.tl_accels:
        pass

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
            description="Generate Chisel wrappers for a given centrifuge-generated function.")

    parser.add_argument('-f', '--func', required=True, help="Name of function to accelerate")
    parser.add_argument('-b', '--base', required=True, help="Base address of function (if tilelink), RoCC index (if rocc)")
    parser.add_argument('-p', '--prefix', default="", help="Optional prefix for function")
    parser.add_argument('-m', '--mode', required=True,
            help="Function integration mode (either 'tl' or 'rocc')")
    parser.add_argument('-s', '--source', required=True, type=pathlib.Path,
            help="Path to the source directory to use when generating (e.g. 'centrifuge/accel/hls_example_func/').")

    args = parser.parse_args()
    scala_dir = args.source / 'src' / 'main' / 'scala'
    scala_dir.mkdir(exist_ok=True)

    import sys
    sys.path.append("..")
    import util

    module_name = pathlib.Path(__file__).stem
    util.setup_logging(module_name, logger)

    ctx = util.initConfig()
    template_dir = util.getOpt('template-dir')

    if args.mode == 'tl':
        funcArgs, retVal = parse_verilog_tl(
                args.source / 'src' / 'main' / 'verilog' / (args.prefix + args.func + "_control_s_axi.v"))
        cWrapper, hWrapper = generateWrapperTL(args.func, args.base, funcArgs, retVal)
    elif args.mode == 'rocc':
        inputs, outputs = parse_verilog_rocc(
                args.source / 'src' / 'main' / 'verilog' / (args.prefix + args.func + ".v"))
        chiselWrapper = generate_chisel_rocc( args.prefix + args.func, args.base, inputs, outputs, scala_dir, template_dir)
    else:
        raise NotImplementedError("Mode '" + args.mode + "' not supported.")

