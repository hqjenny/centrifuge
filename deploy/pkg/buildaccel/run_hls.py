import subprocess
import random
import logging
import json
import os
import shutil
import pathlib
from .. import util

import errno    

from string import Template

logger = logging.getLogger(__name__)
logger.setLevel(logging.NOTSET) 


def modify_verilog(accel):
    """ Update Verilog to make it compatible with chipyard simulators
        1. Add abs path to readmemh 
        2. Replace undefined 'bx signals to 1'b0
    """
    verilog_dir = accel.verilog_dir
    # list all verilog file
    verilog_files = list(verilog_dir.glob('**/*.v'))
    logger.info("\t\tUpdate Verilog Files in {}".format(verilog_dir))
    for verilog_file in verilog_files:
        util.replace_str(verilog_file, '$readmemh("', "$readmemh(\"{}/".format(str(verilog_dir)))
        util.replace_str(verilog_file, "'bx", "1'b0")


def generate_hls_tcl(accel):
    """Generate TCL script to run Vivado HLS"""

    template_dir = util.getOpt('template-dir')
    template_name = 'run_hls_tcl_template'
    with open (template_dir / template_name, 'r') as f:
        template_str= f.read()
     
    t = Template(template_str)
    srcs_str_arr = [] 

    cpp_flag = ' -cflags "-std=c++0x"'
    for src in accel.srcs: 
        src_str = 'add_files ' + str(src)
        if src.suffix == '.cpp':
           src_str += cpp_flag 
        srcs_str_arr.append(src_str)
        
    srcs_str = "\n".join(srcs_str_arr)

    tcl_dict = {
        'PRJ_NAME': 'hls_prj', 
        'PGM': accel.pgm,
        'FUNC': accel.func,
        'PRJ_PREFIX': accel.prefix_id + '_' + accel.pgm + '_',
        'SRCS': srcs_str,
        # TODO allow user to specify in json
        # F1 FPGA part #
        'PART': 'xcvu9p-flgb2104-2-i', 
        # FPGA clock period
        'CLOCK_PERIOD': 10,
    }

    tcl_str = t.substitute(tcl_dict)
    tcl_path = accel.c_dir / 'run_hls.tcl'
    
    with open(tcl_path,'w') as f:
        logger.info("\t\tGenerate TCL script for HLS: {}".format(tcl_path))
        f.write(tcl_str)


def run_hls_cmd(accel):
    subprocess.check_call(['vivado_hls', '-f', 'run_hls.tcl'], cwd=accel.c_dir)


def copy_verilog(accel):
    gen_ver_dir = accel.c_dir / 'hls_prj' / 'solution1' / 'syn' / 'verilog' 
    if not (gen_ver_dir.exists()):
        raise Exception("{} does not exist after running HLS".format(gen_ver_dir))
    util.copytree(gen_ver_dir, accel.verilog_dir)
    logger.info("\t\tCopy\t{} to {}".format(gen_ver_dir, accel.verilog_dir))


def run_hls(accel_conf):
    """Run Vivado HLS"""
    for accel in accel_conf.rocc_accels + accel_conf.tl_accels:
        logger.info("\tRun HLS for {}:".format(accel.prefix_id))
        generate_hls_tcl(accel)
        run_hls_cmd(accel)
        copy_verilog(accel)
        modify_verilog(accel)

