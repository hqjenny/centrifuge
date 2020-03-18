from __future__ import print_function
import re

import random
import logging
import json
import os
import shutil
import errno    
from .. import util
from string import Template
from . import run_hls
from . import generate_chisel
from . import generate_build_sbt

logger = logging.getLogger(__name__)
logger.setLevel(logging.NOTSET) 


def init_proj_dir(accel):
    """Init project directory for accel"""
    dirs = [accel.c_dir, accel.scala_dir, accel.verilog_dir]
    for directory in dirs:
        if not directory.exists():
            util.mkdir_p(directory)


def cp_src(accel):
    """Copy src files"""
    for src_path in accel.srcs: 
        shutil.copy(src_path, accel.c_dir)
        logger.info("\t\tCopy\t{} to {}".format(src_path, accel.c_dir))


def init_accel(accel_conf):
    """Init project directories for accel SoC"""
    for accel in accel_conf.rocc_accels + accel_conf.tl_accels:
        logger.info("\tInitialize {}:".format(accel.prefix_id))
        init_proj_dir(accel)
        cp_src(accel)


def rm_accel(accel_conf):
    """Remove directories for the accel SoC"""
    shutil.rmtree(accel_conf.hw_accel_dir)


def generate_hw(accel_conf):
    """Generate hardware SoC """

    #print("gernerate_hw not implemented. Called with config: ", accel_conf)
    # init project repos
    logger.info("Initialize Generated Hardware Repository {}".format(accel_conf.hw_accel_dir))
    init_accel(accel_conf) 
    run_hls.run_hls(accel_conf)
    generate_chisel.generate_chisel(accel_conf)
    generate_build_sbt.generate_build_sbt(accel_conf)


def clean_hw(accel_conf):
    """Clean hardware SoC Repo """
    rm_accel(accel_conf) 
