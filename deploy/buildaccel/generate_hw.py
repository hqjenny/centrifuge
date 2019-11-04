from __future__ import print_function
import re

import random
import logging
import json
import os
import shutil
import errno    

rootLogger = logging.getLogger()


def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:  # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise

def init_proj_dir(accel):
    """Init project directory for accel"""
    dirs = [accel.c_dir, accel.scala_dir, accel.verilog_dir]
    for directory in dirs:
        if not os.path.exists(directory):
            mkdir_p(directory)

def cp_src(accel):
    """Copy src files"""
    src_files = os.listdir(accel.src_dir)
    for file_name in src_files:
	full_file_name = os.path.join(accel.src_dir, file_name)
	if os.path.isfile(full_file_name):
	    shutil.copy(full_file_name, accel.c_dir)

def init_accel(accel_conf):
    """Init project directories for all accels"""
    for accel in accel_conf.rocc_accels:
        init_proj_dir(accel)
        cp_src(accel)

    for accel in accel_conf.tl_accels:
        init_proj_dir(accel)
        cp_src(accel)


def generate_hw(accel_conf):
    """Generate hardware SoC """

    # init project repos
    init_accel(accel_conf) 


