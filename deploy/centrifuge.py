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
    parser = argparse.ArgumentParser(description='Centrifuge Script')
    parser.add_argument('task', 
                        type=str,
                        help='Management task to run.', 
                        choices=[
                            'generate_all',
                            'clean_all',
                            'generate_hw',
                            'clean_hw',
                            'generate_sw',
                            'clean_sw',
                            'run_f1',
                            'run_sim',
                            ], 
                        )
    parser.add_argument('-c', 
                        '--accelconfigfile', 
                        type=pathlib.Path,
                        help='Path to accelerator SoC config JSON file.',
                        required=True
                        )
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
        buildaccel.generate_hw(accel_config)
    elif args.task == 'clean_hw':
        buildaccel.clean_hw(accel_config)
    elif args.task == 'generate_sw':
        buildsw.generateSW(accel_config)
    else:
        print("Command: " + str(args.task) + " not yet implemented")

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
    except Exception as e:
        # log all exceptions that make it this far
        rootLogger.exception("Fatal error: " + str(e))
        exitcode = 1
    finally:
        rootLogger.info("""The full log of this run is:\n{logdir}/{runname}.log""".format(logdir=util.getOpt('log-dir'), runname=util.getOpt('run-name')))
        exit(exitcode)
