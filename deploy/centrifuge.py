#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK

from __future__ import with_statement, print_function
import sys
import os
import argparse
from time import sleep, strftime, gmtime
import logging
import random
import string  
import argcomplete
import pathlib

# centrifuge 
import pkg.util as util 
import pkg.buildaccel as buildaccel
import pkg.buildsw as buildsw

from os.path import dirname as up

def construct_centrifuge_argparser():
    # parse command line args
    parser = argparse.ArgumentParser(description='Centrifuge Script', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('task', 
                        type=str,
                        help="""Management task to run. 
Options: 
        generate_all -- Generate all HW/SW interfaces, configs, and scripts for the accelerator SoC
        clean_all -- Delete accelerator directory
        generate_hw -- Rerun accelerator HW/SW interface generation  
        clean_hw -- Clean accelerator hardware directory
        generate_sw -- Rerun C wrapper generation (triggers accel_hls to run)
        clean_sw -- Clean accelerator software directory
        run_firesim -- Run FireSim
        run_vcs -- Run vcs simulation
        run_verilator -- Run verilator simulation
""", 
                        choices=[
                            'generate_all',
                            'clean_all',
                            'generate_hw',
                            'clean_hw',
                            'generate_sw',
                            'clean_sw',
                            'run_firesim',
                            'run_vcs',
                            'run_verilator',
                            ], 
                        )
    parser.add_argument('-c', 
                        '--accelconfigfile', 
                        type=pathlib.Path,
                        help='Path to accelerator SoC config JSON file.',
                        required=True
                        )
    parser.add_argument('-p', 
                        '--swfile', 
                        type=pathlib.Path,
                        help='Path to bare-metal software binary.',
                        )
    parser.add_argument('-t', 
                        '--subtask', 
                        type=str,
                        help="""Subtask to run.
Options:
    generate_hw 
        hls -- Run HLS to generate accelerator
        chisel -- Generate CHISEL wrapper
        build_sbt -- Generate build.sbt
        config -- Generate HLSConfig.scala file under chipyard
        f1_scripts -- Generate scripts to run FireSim on Amazon F1 FPGAs

    generate_sw
        bm -- Generate baremetal wrappers 

    run_firesim
        f1_scripts -- Regenerate scripts for including HLS generated Verilog in FireSim
        xsim_scripts -- Regenerate scripts for including HLS generated Verilog in FireSim XSim
        build/build_recipes/hwdb/runtime -- Generate corresponding example FireSim configurations 
        task -- Run FireSim task with generated configurations. 
                Options:
                        buildafi/shareagfi/launchrunfarm/terminaterunfarm/infrasetup/boot/kill/runworkload/runcheck

        if --subtask/-t is not specifed, will run 
            fl_scripts
            build_recipes
            build
            hwdb
            runtime

    run_vcs / run_verilator
        clean -- Clean the simulation files
        debug -- Enable debug mode for the vcs simulation 
        append_verilog -- Append HLS-generated Verilog to Top.v for simulation
        run_bm_sw -- Run bare-metal software binary specified by --swfile/-p to run in compiled vcs simulator 

        if --subtask/-t is not specifed, will run 
            clean
            debug
            append_verilog 
            debug


""",
                        )

    parser.add_argument('--with_nic', help='FireSim Config with NIC',
        action='store_true')
    parser.add_argument('--s3_bucket', help='FireSim S3 Bucket Name',
        type=str,
        default='firesim-978989785248')
    parser.add_argument('--agfi', help='FireSim AGFI ID',
        type=str,
        default='PLACEHOLDER')
    parser.add_argument('--workload', help='FireSim SW Workload Definition JSON',
        type=str,
        default='PLACEHOLDER')
    parser.add_argument('-a', '--firesim_task', help='FireSim Task',
        type=str)



    argcomplete.autocomplete(parser)
    return parser.parse_args()


def initLogging():
    """Set up logging for this run. Assumes that util.initConfig() has been called already."""
    rootLogger = logging.getLogger()
    rootLogger.setLevel(logging.NOTSET) # capture everything

    # log to file
    full_log_filename = util.getOpt("log-dir") / (util.getOpt("run-name") + ".log")
    fileHandler = logging.FileHandler(full_log_filename)
    # formatting for log to file
    logFormatter = logging.Formatter("%(asctime)s [%(funcName)-12.12s] [%(levelname)-5.5s]  %(message)s")
    fileHandler.setFormatter(logFormatter)
    fileHandler.setLevel(logging.NOTSET) # log everything to file
    rootLogger.addHandler(fileHandler)

    # log to stdout, without special formatting
    consoleHandler = logging.StreamHandler(stream=sys.stdout)
    consoleHandler.setLevel(logging.INFO) # show only INFO and greater in console
    rootLogger.addHandler(consoleHandler)
    return rootLogger


def main(args):
    """ Main function for FireSim manager. """

    # load accel.json 
    accel_config = util.AccelConfig(args.accelconfigfile, 
                                    util.getOpt('chipyard-dir'), 
                                    util.getOpt('cf-dir'),
                                    util.getOpt('genhw-dir'))

    # print the info
    accel_config.info()

    # tasks that have a special config/dispatch setup
    if args.task == 'generate_hw':
        buildaccel.generate_hw(accel_config, args.subtask)
    elif args.task == 'clean_hw':
        buildaccel.clean_hw(accel_config)
    elif args.task == 'generate_sw':
        buildsw.generateSW(accel_config)
    elif args.task == 'run_vcs':
        buildaccel.run_vcs(accel_config, args.subtask, args.swfile)
    elif args.task == 'run_verilator':
        buildaccel.run_verilator(accel_config, args.subtask)
    elif args.task == 'run_firesim':
        buildaccel.run_firesim(accel_config, args)
    else:
        print("Command: " + str(args.task) + " not yet implemented")
        raise NotImplementedError()

if __name__ == '__main__':

    args = construct_centrifuge_argparser()

    # Make all paths absolute as early as possible
    args.accelconfigfile = args.accelconfigfile.resolve()

    ctx = util.initConfig()
    ctx.setRunName(args.accelconfigfile, args.task)
    rootLogger = initLogging()

    exitcode = 0
    try:
        main(args)
    except:
        # log all exceptions that make it this far
        rootLogger.exception("Fatal error.")
        exitcode = 1
    finally:
        rootLogger.info("""The full log of this run is:\n{logdir}/{runname}.log""".format(logdir=util.getOpt('log-dir'), runname=util.getOpt('run-name')))
        exit(exitcode)
