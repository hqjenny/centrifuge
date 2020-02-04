#!/usr/bin/env python2
# PYTHON_ARGCOMPLETE_OK

# REQUIRES PYTHON2, because fabric requires python2

from __future__ import with_statement, print_function
import sys
import os
import signal
import argparse
from time import sleep, strftime, gmtime
import logging
import random
import string  
import argcomplete


# centrifuge 
from parseconfig import AccelConfig

from util.streamlogger import StreamLogger
from os.path import dirname as up

def construct_centrifuge_argparser():
    # parse command line args
    parser = argparse.ArgumentParser(description='Centrifuge Script')
    parser.add_argument('task', type=str,
                        help='Management task to run.', choices=[
                            'generate_all',
                            'clean_all',
                            'generate_hw',
                            'clean_hw',
                            'generate_sw',
                            'clean_sw',
                            ])
    parser.add_argument('-c', '--accelconfigfile', type=str,
                        help='Path to accelerator SoC config JSON file. Defaults to accel.json',
                        default='accel.json'
                        )
    argcomplete.autocomplete(parser)
    return parser.parse_args()

def initLogging():
    """Set up logging for this run. Assumes that util.initConfig() has been called already."""
    rootLogger = logging.getLogger()
    rootLogger.setLevel(logging.NOTSET) # capture everything

    # log to file
    full_log_filename = util.getOpt("log-dir") / util.getOpt("run-name")
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
    accel_config = AccelConfig(args.accelconfigfile, util.getOpt('chipyard-dir'), util.getOpt('cf-dir'))
    
    # print the info
    accel_config.info()

    # tasks that have a special config/dispatch setup
    if args.task == 'generate_hw':
       pass 

if __name__ == '__main__':

    args = construct_centrifuge_argparser()

    ctx = util.initConfig()
    ctx.setRunName(
    rootLogger = initLogging(args.accelconfigfile, args.task)

    os.chdir(util.getOpt('cf-dir'))
    exitcode = 0
    try:
        main(args)
    except:
        # log all exceptions that make it this far
        rootLogger.exception("Fatal error.")
        exitcode = 1
    finally:
        rootLogger.info("""The full log of this run is:\n{logdir}/{runname}.log""".format(logdir=util.getOpt('log-dir'), runname=util.getOpt('run-name'))
        exit(exitcode)
