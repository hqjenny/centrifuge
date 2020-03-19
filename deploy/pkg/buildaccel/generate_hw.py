from __future__ import print_function
import re

import subprocess
import random
import logging
import json
import os
import shutil
import errno    
from .. import util
from string import Template

rootLogger = logging.getLogger()

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:  # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise

def copytree(src, dst, symlinks=False, ignore=None):
    for item in src.glob('*'):
        s = src / item.name
        d = dst / item.name
        if s.is_dir():
            shutil.copytree(s, d, symlinks, ignore)
        else:
            shutil.copy2(s, d)

def init_proj_dir(accel):
    """Init project directory for accel"""
    dirs = [accel.c_dir, accel.scala_dir, accel.verilog_dir]
    for directory in dirs:
        if not directory.exists():
            mkdir_p(directory)

def cp_src(accel):
    """Copy src files"""
    for src_path in accel.srcs: 
        shutil.copy(src_path, accel.c_dir)
        rootLogger.info("\t\tCopy\t{} to {}".format(src_path, accel.c_dir))

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
            'prj_name' : 'hls_prj', 
            'pgm' : accel.pgm,
            'func': accel.func,
            'prj_prefix' : accel.prefix_id + '_' + accel.pgm + '_',
            'srcs' : srcs_str,
            # TODO allow user to specify in json
            # F1 FPGA part #
            'part' : 'xcvu9p-flgb2104-2-i', 
            # FPGA clock period
            'clock_period' : 10,
            }

    tcl_str = t.substitute(tcl_dict)
    tcl_path = accel.c_dir / 'run_hls.tcl'
    
    with open(tcl_path,'w') as f:
        rootLogger.info("\t\tGenerate TCL script for HLS: {}".format(tcl_path))
        f.write(tcl_str)

def run_hls_cmd(accel):
    subprocess.check_call(['vivado_hls', '-f', 'run_hls.tcl'], cwd=accel.c_dir)

#TODO 
def process_verilog(accel):
    pass
    # TODO use aes testbench to test 
    # 1. my $perl_cmd = "perl -p -i -e 's/\$readmemh\\\(\\\"\\\.\/\$readmemh(\\\"$vivado_dir_escape/g' *";
    # 2. $perl_cmd = "perl -p -i -e \"s/'bx/1'b0/g\" *";

def copy_verilog(accel):
    gen_ver_dir = accel.c_dir / 'hls_prj/solution1/syn/verilog/'
    if not (gen_ver_dir.exists()):
        raise Exception("{} does not exist after running HLS".format(gen_ver_dir))
    copytree(gen_ver_dir, accel.verilog_dir)
    rootLogger.info("\t\tCopy\t{} to {}".format(gen_ver_dir, accel.verilog_dir))

def run_hls(accel_conf):
    """Run Vivado HLS"""
    for accel in accel_conf.rocc_accels + accel_conf.tl_accels:
        rootLogger.info("\tRun HLS for {}:".format(accel.prefix_id))
        generate_hls_tcl(accel)
        run_hls_cmd(accel)
        copy_verilog(accel)

def init_accel(accel_conf):
    """Init project directories for accel SoC"""
    for accel in accel_conf.rocc_accels + accel_conf.tl_accels:
        rootLogger.info("\tInitialize {}:".format(accel.prefix_id))
        init_proj_dir(accel)
        cp_src(accel)

def rm_accel(accel_conf):
    """Remove directories for the accel SoC"""
    shutil.rmtree(accel_conf.hw_accel_dir)

def generate_hw(accel_conf):
    """Generate hardware SoC """

    #print("gernerate_hw not implemented. Called with config: ", accel_conf)
    # init project repos
    rootLogger.info("Initialize Generated Hardware Repository {}".format(accel_conf.hw_accel_dir))
    init_accel(accel_conf) 
    run_hls(accel_conf)

def clean_hw(accel_conf):
    """Clean hardware SoC Repo """
    rm_accel(accel_conf) 
