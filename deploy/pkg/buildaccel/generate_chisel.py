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

        matchApSignals = reApSignals.match(k)
        if not matchApSignals and not matchPointerSignals:
            arg_name = k
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
        return "    val scalar_io = HeterogeneousBag(scalar_io_dataWidths.map(w => Input(UInt(w.W))))\n"
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
        return "val ap_return = UInt(4.W)\n"


def generate_vals(io, width):
    if width == 1:
        val = "{}(Bool())".format(io)
    else:
        val = "{}(UInt({}.W))".format(io, width)
    return val


def generate_params(params):
    params_arr = []

    # Parameters
    for k, v in params.items():
        param_str = "val {} = {}".format(k, v)
        params_arr.append(param_str)
    return params_arr


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


def generate_opt_ap_signals(inputs, outputs):
    ret_str = ""
    if 'ap_return' in list(outputs.keys()):
        ret_str += "    io.ap.rtn := bb.io.ap_return\n"
    if 'ap_rst' in list(inputs.keys()):
        ret_str += "    bb.io.ap_rst := reset\n"
    if 'ap_clk' in list(inputs.keys()):
        ret_str += "    bb.io.ap_clk := clock\n"
    if 'ap_rst_n' in list(inputs.keys()):
        ret_str += "    bb.io.ap_rst_n := !reset.asBool()\n"
    return ret_str


def generate_rocc_assignment(input_info):
    scalar_template = Template("    bb.io.${ARG} := io.scalar_io($IDX)\n")
    ptr_template = Template("""    io.ap_bus($IDX).req.din := bb.io.${ARG}_req_din
    bb.io.${ARG}_req_full_n := io.ap_bus($IDX).req_full_n
    io.ap_bus($IDX).req_write := bb.io.${ARG}_req_write
    bb.io.${ARG}_rsp_empty_n := io.ap_bus($IDX).rsp_empty_n
    io.ap_bus($IDX).rsp_read := bb.io.${ARG}_rsp_read
    io.ap_bus($IDX).req.address := bb.io.${ARG}_address
    bb.io.${ARG}_datain := io.ap_bus($IDX).rsp.datain
    io.ap_bus($IDX).req.dataout := bb.io.${ARG}_dataout
    io.ap_bus($IDX).req.size := bb.io.${ARG}_size
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
            size = None
            if argWidthMatch:
                end = argWidthMatch.group(1)
                endMatch = re.match(r"(\S+) - 1", end)
                if endMatch:
                    size = endMatch.group(1)
                    intMatch = re.match(r"\d+", size)
                    if intMatch:
                        end = size - 1
                        size = None
                start = argWidthMatch.group(2)
                argName = argWidthMatch.group(3)
            if size is None:
                width = int(end) - int(start) + 1
            else:
                width = size
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

    logger.info("Inputs: {}".format(inputs))
    logger.info("Outputs: {}".format(outputs))
    return (inputs, outputs)


def generate_chisel_rocc(func, idx, inputs, outputs, scala_dir, template_dir):

    ##########################################################
    logger.info("Generating RoCC BlackBox file ...")
    template_name = 'chisel_rocc_blackbox_scala_template'
    template_path = template_dir / template_name

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
    ap_return_rst_clk_str = generate_opt_ap_signals(inputs, outputs)
    signal_assignment_str = generate_rocc_assignment(input_info)

    chisel_dict = {
        "FUNC": func,
        "ARGS": args_str,
        'SCALAR_DATA_WIDTH_ARR': info_dict['SCALAR_DATA_WIDTH_ARR'],
        'SCALAR_IDX_ARR': info_dict['SCALAR_IDX_ARR'],
        'PTR_ADDR_WIDTH_ARR': info_dict['PTR_ADDR_WIDTH_ARR'],
        'PTR_DATA_WIDTH_ARR': info_dict['PTR_DATA_WIDTH_ARR'],
        'PTR_IDX_ARR': info_dict['PTR_IDX_ARR'],
        'RETURN_WIDTH': return_width,
        'SCALAR_IO': scalar_io_str,
        'AP_RETURN_RST_CLK': ap_return_rst_clk_str,
        'SIGNAL_ASSIGNMENT': signal_assignment_str,

    }
    scala_path = scala_dir / pathlib.Path(func + '_blackbox.scala')
    util.generate_file(template_path, chisel_dict, scala_path)
    logger.info("\t\tGenerate rocc_blackbox code in CHISEL: {}".format(scala_path))

    ##########################################################
    logger.info("Generating RoCC Control file ...")
    template_name = 'chisel_rocc_accel_scala_template'
    template_path = template_dir / template_name

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

    scala_path = scala_dir / pathlib.Path(func + '_accel.scala')
    util.generate_file(template_path, chisel_dict, scala_path)
    logger.info("\t\tGenerate rocc_accel code in CHISEL: {}".format(scala_path))

    ##########################################################
    logger.info("Copying Vivado HLS Interface file ...");
    src_path = template_dir / 'ap_bus_scala_template'
    dst_path = scala_dir / 'ap_bus.scala'
    shutil.copy(str(src_path), str(dst_path))

    ##########################################################
    logger.info("Copying ROCC Memory Controller file ...");
    src_path = template_dir / 'memControllerComponents_scala_template'
    dst_path = scala_dir / 'memControllerComponents.scala'
    shutil.copy(str(src_path), str(dst_path))

    ##########################################################
    logger.info("Copying RoCC Controller Utilities file ...");
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

    logger.info("Parsing: {}".format(vpath))
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
                        params[param] = width
                        busMatch = re.match(r"C_M_AXI_(\S+)_DATA_WIDTH", param)
                        if busMatch:
                            bus = busMatch.group(1).lower()
                            buses[bus] = {'width': width}

    logger.info("Inputs: {}".format(inputs))
    logger.info("Outputs: {}".format(outputs))
    logger.info("Paramters: {}".format(params))
    logger.info("Buses: {}".format(buses))
    return (inputs, outputs, params, buses)


def generate_tl_assignment(buses):
    ret_str = ""
    template = Template("""
    val node_${BUS_NAME} = AXI4MasterNode(Seq(AXI4MasterPortParameters(
        masters = Seq(AXI4MasterParameters(
            name = "axil_hub_mem_out_${IDX}",
            id = IdRange(0, numInFlight),
            aligned = true,
        maxFlight = Some(8)
      ))
      )
    ))
""")

    for idx, (k, v) in enumerate(buses.items()):
        d = {'BUS_NAME': k, 'IDX': idx}
        ret_str += template.substitute(d)

    return ret_str


def generate_AXI_signal(matchInput, template):
    assert(matchInput.group(1) is not None)
    assert(matchInput.group(2) is not None)
    bus = matchInput.group(1)
    signal_type = matchInput.group(2).lower()
    return template.format(bus, signal_type)


def construct_axi_regex(regex):
    axi_types = ['m', 's']
    ret_dict = {}
    for axi_type in axi_types:
       ret_dict[axi_type] = re.compile(axi_type + regex)
    return ret_dict


def generate_tl_module_stmt(inputs, outputs, buses):
    ret_str = ""

    bus_stmt_arr = []
    for k, _ in buses.items():
        bus_stmt = "val (out_{0}, edge_{0}) = outer.node_{0}.out(0)".format(k)
        bus_stmt_arr.append(bus_stmt)
    ret_str += "\n    ".join(bus_stmt_arr)
    ret_str += "\n"

    ret_str += generate_opt_ap_signals(inputs, outputs)
    ret_str += "\n"

    reAXI = re.compile('^(m_axi|s_axi)\S+$')

    # Input Signals Regex
    reAWWARREADY = construct_axi_regex('_axi_(.*)_(AW|W|AR)READY$')
    reRBVALID = construct_axi_regex('_axi_(.*)_(R|B)VALID$')
    reRDATA = construct_axi_regex('_axi_(.*)_(R)DATA$')
    reRLAST = construct_axi_regex('_axi_(.*)_(R)LAST$')
    reRBID = construct_axi_regex('_axi_(.*)_(R|B)ID$')
    reRBUSER = construct_axi_regex('_axi_(.*)_(R|B)USER$')
    reRBRESP = construct_axi_regex('_axi_(.*)_(R|B)RESP$')

    reAWWARVALID = construct_axi_regex('_axi_(.*)_(AW|W|AR)VALID$')
    reAWARADDR = construct_axi_regex('_axi_(.*)_(AW|AR)ADDR$')
    reWDATA = construct_axi_regex('_axi_(.*)_(W)DATA$')
    reWSTRB = construct_axi_regex('_axi_(.*)_(W)STRB$')
    reRBREADY = construct_axi_regex('_axi_(.*)_(R|B)READY$')
    reWLAST = construct_axi_regex('_axi_(.*)_(W)LAST$')
    reWID = construct_axi_regex('_axi_(.*)_(W)ID$')
    reWUSER = construct_axi_regex('_axi_(.*)_(W)USER$')

    reAWARID = construct_axi_regex('_axi_(.*)_(AW|AR)ID')
    reAWARLEN = construct_axi_regex('_axi_(.*)_(AW|AR)LEN$')
    reAWARSIZE = construct_axi_regex('_axi_(.*)_(AW|AR)SIZE$')
    reAWARBURST = construct_axi_regex('_axi_(.*)_(AW|AR)BURST$')
    reAWARLOCK = construct_axi_regex('_axi_(.*)_(AW|AR)LOCK$')
    reAWARCACHE = construct_axi_regex('_axi_(.*)_(AW|AR)CACHE$')
    reAWARPROT = construct_axi_regex('_axi_(.*)_(AW|AR)PROT$')
    reAWARQOS = construct_axi_regex('_axi_(.*)_(AW|AR)QOS$')
    reAWARREGION = construct_axi_regex('_axi_(.*)_(AW|AR)REGION$')
    reAWARUSER = construct_axi_regex('_axi_(.*)_(AW|AR)USER$')

    bus_assign_arr = []
    for k, v in inputs.items():
        matchAXI = reAXI.match(k)
        if matchAXI:
            matchAWWARREADY = reAWWARREADY['m'].match(k)
            matchRBVALID = reRBVALID['m'].match(k)
            matchRDATA = reRDATA['m'].match(k)
            matchRLAST = reRLAST['m'].match(k)
            matchRBID = reRBID['m'].match(k)
            matchRBUSER = reRBUSER['m'].match(k)
            matchRBRESP = reRBRESP['m'].match(k)

            matchAWWARVALID = reAWWARVALID['s'].match(k)
            matchAWARADDR = reAWARADDR['s'].match(k)
            matchWDATA = reWDATA['s'].match(k)
            matchWSTRB = reWSTRB['s'].match(k)
            matchRBREADY = reRBREADY['s'].match(k)

            assign_str = None
            in_str = "bb.io." + k
            if matchAWWARREADY:
                assign_str = generate_AXI_signal(matchAWWARREADY,
                    in_str + " := out_{0}.{1}.ready")
            elif matchRBVALID:
                assign_str = generate_AXI_signal(matchRBVALID,
                    in_str + " := out_{0}.{1}.valid")
            elif matchRDATA:
                assign_str = generate_AXI_signal(matchRDATA,
                    in_str + " := out_{0}.{1}.bits.data")
            elif matchRLAST:
                assign_str = generate_AXI_signal(matchRLAST,
                    in_str + " := out_{0}.{1}.bits.last")
            elif matchRBID:
                assign_str = generate_AXI_signal(matchRBID,
                    in_str + " := out_{0}.{1}.bits.id")
            elif matchRBUSER: # omit user signal
                assign_str = ""
            elif matchRBRESP:
                assign_str = generate_AXI_signal(matchRBRESP,
                    in_str + " := out_{0}.{1}.bits.resp")
            elif matchAWWARVALID:
                assign_str = generate_AXI_signal(matchAWWARVALID,
                    in_str + " := slave_in.{1}.valid")
            elif matchAWARADDR:
                assign_str = generate_AXI_signal(matchAWARADDR,
                    in_str + " := slave_in.{1}.bits.addr")
            elif matchWDATA:
                assign_str = generate_AXI_signal(matchWDATA,
                    in_str + " := slave_in.{1}.bits.data")
            elif matchWSTRB:
                assign_str = generate_AXI_signal(matchWSTRB,
                    in_str + " := slave_in.{1}.bits.strb")
            elif matchRBREADY:
                assign_str = generate_AXI_signal(matchRBREADY,
                    in_str + " := slave_in.{1}.ready")
            assert(assign_str is not None)
            bus_assign_arr.append(assign_str)

    for k, v in outputs.items():
        matchAXI = reAXI.match(k)
        if matchAXI:
            matchAWWARREADY = reAWWARREADY['s'].match(k)
            matchRBVALID = reRBVALID['s'].match(k)
            matchRDATA = reRDATA['s'].match(k)
            matchRBRESP = reRBRESP['s'].match(k)

            matchAWWARVALID = reAWWARVALID['m'].match(k)
            matchRBREADY = reRBREADY['m'].match(k)
            matchAWARADDR = reAWARADDR['m'].match(k)
            matchAWARID = reAWARID['m'].match(k)
            matchAWARLEN = reAWARLEN['m'].match(k)

            matchAWARSIZE = reAWARSIZE['m'].match(k)
            matchAWARBURST = reAWARBURST['m'].match(k)
            matchAWARLOCK = reAWARLOCK['m'].match(k)
            matchAWARCACHE = reAWARCACHE['m'].match(k)
            matchAWARPROT = reAWARPROT['m'].match(k)

            matchAWARQOS = reAWARQOS['m'].match(k)
            matchAWARREGION = reAWARREGION['m'].match(k)
            matchAWARUSER = reAWARUSER['m'].match(k)
            matchWDATA = reWDATA['m'].match(k)
            matchWSTRB = reWSTRB['m'].match(k)
            matchWLAST = reWLAST['m'].match(k)
            matchWID = reWID['m'].match(k)
            matchWUSER = reWUSER['m'].match(k)

            assign_str = None
            out_str = "bb.io." + k
            if matchAWWARVALID:
                assign_str = generate_AXI_signal(matchAWWARVALID,
                    "out_{0}.{1}.valid := " + out_str)
            elif matchRBREADY:
                assign_str = generate_AXI_signal(matchRBREADY,
                    "out_{0}.{1}.ready :=" + out_str)
            elif matchAWARADDR:
                assign_str = generate_AXI_signal(matchAWARADDR,
                    "out_{0}.{1}.bits.addr := " + out_str)
            elif matchAWARID:
                assign_str = generate_AXI_signal(matchAWARID,
                    "out_{0}.{1}.bits.id := " + out_str)
            elif matchAWARLEN:
                assign_str = generate_AXI_signal(matchAWARLEN,
                    "out_{0}.{1}.bits.len := " + out_str)
            elif matchAWARSIZE:
                assign_str = generate_AXI_signal(matchAWARSIZE,
                    "out_{0}.{1}.bits.size := " + out_str)
            elif matchAWARBURST:
                assign_str = generate_AXI_signal(matchAWARBURST,
                    "out_{0}.{1}.bits.burst := " + out_str)
            elif matchAWARLOCK:
                assign_str = generate_AXI_signal(matchAWARLOCK,
                    "out_{0}.{1}.bits.lock := " + out_str)
            elif matchAWARCACHE:
                assign_str = generate_AXI_signal(matchAWARCACHE,
                    "out_{0}.{1}.bits.cache := " + out_str)
            elif matchAWARPROT:
                assign_str = generate_AXI_signal(matchAWARPROT,
                    "out_{0}.{1}.bits.prot := " + out_str)
            elif matchAWARQOS:
                assign_str = generate_AXI_signal(matchAWARQOS,
                    "out_{0}.{1}.bits.qos := " + out_str)
            elif matchAWARREGION:
                assign_str = generate_AXI_signal(matchAWARREGION,
                    "//out_{0}.{1}.bits.region := " + out_str)
            elif matchAWARUSER:
                assign_str = ""
            elif matchWDATA:
                assign_str = generate_AXI_signal(matchWDATA,
                    "out_{0}.{1}.bits.data := " + out_str)
            elif matchWSTRB:
                assign_str = generate_AXI_signal(matchWSTRB,
                    "out_{0}.{1}.bits.strb := " + out_str)
            elif matchWLAST:
                assign_str = generate_AXI_signal(matchWLAST,
                    "out_{0}.{1}.bits.last := " + out_str)
            elif matchWID:
                # TODO check if this is needed
                #assign_str = generate_AXI_signal(matchWID,
                #    "out_{0}.{1}.bits.id := " + out_str)
                # No such signal in TLtoAXI4
                assign_str = ""
            elif matchWUSER:
                assign_str = ""
            elif matchAWWARREADY:
                assign_str = generate_AXI_signal(matchAWWARREADY,
                    "slave_in.{1}.ready := " + out_str)
            elif matchRBVALID:
                assign_str = generate_AXI_signal(matchRBVALID,
                    "slave_in.{1}.valid := " + out_str)
            elif matchRDATA:
                assign_str = generate_AXI_signal(matchRDATA,
                    "slave_in.{1}.bits.data := " + out_str)
            elif matchRBRESP:
                assign_str = generate_AXI_signal(matchRBRESP,
                    "slave_in.{1}.bits.resp := " + out_str)

            assert(assign_str is not None)
            bus_assign_arr.append(assign_str)
    ret_str += "    "
    ret_str += "\n    ".join(bus_assign_arr)


    # add return stmt if any
    if 'ap_return' in list(outputs.keys()):
        ret_str += "\n    val ap_return = accel.io.ap.rtn\n"

    return ret_str


def generate_tl_trait_stmt(func, buses):
    ret_str = ""
    template = Template("""
    sbus.coupleFrom(s"port_named_$$axi_m_portName") {
      ( _
        := TLBuffer(BufferParams.default)
        := TLFIFOFixer(TLFIFOFixer.all)
        := TLWidthWidget(${M_AXI_DATA_WIDTH} >> 3)
        := AXI4ToTL()
        := AXI4UserYanker(Some(8))
        := AXI4Fragmenter()
        := AXI4IdIndexer(1)
        := hls_tl0_vadd_tl_vadd_accel.node_gmem0
      )
    }
""")

    for k, v in buses.items():
        d = {'FUNC': func, 'BUS_NAME': k, 'M_AXI_DATA_WIDTH': v['width']}
        ret_str += template.substitute(d)
    return ret_str


def generate_chisel_tl(func, idx, inputs, outputs, params, buses, scala_dir, template_dir):
    ##########################################################
    logger.info("Generating TL BlackBox file ...")
    template_name = 'chisel_tl_blackbox_scala_template'
    template_path = template_dir / template_name

    # Generate parameters
    params_arr = generate_params(params)
    params_str = "\n    ".join(params_arr)

    # Generate arguments
    args_arr = generate_args(inputs, outputs)
    args_str = "\n        ".join(args_arr)

    chisel_dict = {
        "FUNC": func,
        "PARAMS": params_str,
        "ARGS": args_str,
    }
    scala_path = scala_dir / pathlib.Path(func + '_blackbox.scala')
    util.generate_file(template_path, chisel_dict, scala_path)
    logger.info("\t\tGenerate tl_blackbox code in CHISEL: {}".format(scala_path))

    ##########################################################
    logger.info("Generating TL Control file ...")
    template_name = 'chisel_tl_accel_scala_template'
    template_path = template_dir / template_name

    # Add dummy bus
    if len(buses) < 1:
        buses['gmem_dummy'] = {'width': 32}

    axi_master_stmt_str  = generate_tl_assignment(buses)
    axi_module_stmt_str = generate_tl_module_stmt(inputs, outputs, buses)
    # TODO test multi bundles
    axi_trait_stmt_str = generate_tl_trait_stmt(func, buses)

    chisel_dict = {
        "FUNC": func,
        "BASE_ADDR": idx,
        "S_AXI_DATA_WIDTH": params['C_S_AXI_DATA_WIDTH'],
        "AXI_MASTER_STMT": axi_master_stmt_str,
        "AXI_MODULE_STMT": axi_module_stmt_str,
        "AXI_TRAIT_STMT": axi_trait_stmt_str,
    }
    scala_path = scala_dir / pathlib.Path(func + '_accel.scala')
    util.generate_file(template_path, chisel_dict, scala_path)
    logger.info("\t\tGenerate tl_accel code in CHISEL: {}".format(scala_path))

def generate_chisel(accel_conf):
    from .. import util
    template_dir = util.getOpt('template-dir')
    for accel in accel_conf.rocc_accels:
        logger.info("\tRun CHISEL generation for {}:".format(accel.name))
        inputs, outputs = parse_verilog_rocc(
                accel.verilog_dir / (accel.name + ".v"))
        generate_chisel_rocc( accel.name, accel.rocc_insn_id, inputs, outputs, accel.scala_dir, template_dir)

    for accel in accel_conf.tl_accels:
        logger.info("\tRun CHISEL generation for {}:".format(accel.name))
        inputs, outputs, params, buses = parse_verilog_tl(
                accel.verilog_dir / (accel.name + ".v"))
        generate_chisel_tl( accel.name, accel.base_addr, inputs, outputs, params, buses, accel.scala_dir, template_dir)


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
        inputs, outputs, params, buses = parse_verilog_tl(
                args.source / 'src' / 'main' / 'verilog' / (args.prefix + args.func + ".v"))
        generate_chisel_tl(args.prefix + args.func, args.base, inputs, outputs, params, buses, scala_dir, template_dir)
    elif args.mode == 'rocc':
        inputs, outputs = parse_verilog_rocc(
                args.source / 'src' / 'main' / 'verilog' / (args.prefix + args.func + ".v"))
        generate_chisel_rocc( args.prefix + args.func, args.base, inputs, outputs, scala_dir, template_dir)
    else:
        raise NotImplementedError("Mode '" + args.mode + "' not supported.")

