import subprocess
import logging
from .. import util

logger = logging.getLogger(__name__)
logger.setLevel(logging.NOTSET) 

def run_bm_sw(accel_conf, simulator, bm_sw_path):
    logger.info("\t\tRunning {} Simulation for {}".format(simulator, bm_sw_path))

    sims_dir = accel_conf.sims_dir / simulator
    cmd = "simv-chipyard-{}-debug {}".format(accel_conf.CONFIG, str(bm_sw_path))
    logger.info("\t\tCommand: {}".format(cmd))
    
    cmd_arr = cmd.split()
    subprocess.check_call(cmd_arr, cwd=sims_dir)

def compile_sim_cmd(accel_conf, simulator, cmd=""):
    clean_str = "";
    
    sims_dir = accel_conf.sims_dir / simulator
    cmd = "make {} CONFIG={} TOP={}".format(cmd, accel_conf.CONFIG, accel_conf.TOP);
    logger.info("\t\tCompiling {} Simulation in {}".format(simulator, sims_dir))
    logger.info("\t\tCommand: {}".format(cmd))

    cmd_arr = cmd.split()
    subprocess.check_call(cmd_arr, cwd=sims_dir)


def run_sim(accel_conf, simulator, subtask, bm_sw_path=None):
    if subtask is None:
        compile_sim_cmd(accel_conf, simulator, 'clean')
        compile_sim_cmd(accel_conf, simulator, 'debug')
        append_verilog_to_top_v(accel_conf, simulator)
        compile_sim_cmd(accel_conf, simulator, 'debug')
    else:
        if subtask == 'clean':
            compile_sim_cmd(accel_conf, simulator, 'clean')
        elif subtask == 'debug':
            compile_sim_cmd(accel_conf, simulator, 'debug')
        elif subtask == 'append_verilog':
            append_verilog_to_top_v(accel_conf, simulator)
        elif subtask == 'run_bm_sw':
            if bm_sw_path is None:
                import argparse
                raise argparse.ArgumentError("Please specify software baremetal binary path with --swfile/-p!")
            run_bm_sw(accel_conf, simulator, bm_sw_path)
        else:
            raise NotImplementedError()


def append_verilog_to_top_v(accel_conf, simulator):
    top_v_prefix = "chipyard.TestHarness.{}".format(accel_conf.CONFIG)
    top_v_file = "{}.top.v".format(top_v_prefix)
    top_v_path = accel_conf.sims_dir / simulator / "generated-src" / top_v_prefix / top_v_file 

    logger.info("\t\tAppend Verilog Files to {}".format(top_v_path))
    for accel in accel_conf.rocc_accels + accel_conf.tl_accels:
        verilog_dir = accel.verilog_dir
        # list all verilog file
        verilog_files = list(verilog_dir.glob('**/*.v'))
        for verilog_file in verilog_files:
            util.append_to_file(verilog_file, top_v_path)


def run_vcs(accel_conf, subtask, bm_sw_path):
    run_sim(accel_conf, 'vcs', subtask, bm_sw_path)


def run_verilator(accel_conf, subtask, bm_sw_path):
    run_sim(accel_conf, 'verilator', subtask, bm_sw_path)


def run_firesim(accel_conf):
    pass
