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

def main(args):
    """ Main function for FireSim manager. """

    # load accel.json 
    accel_config = AccelConfig(args.accelconfigfile, chipyard_dir, centrifuge_dir)
    
    # print the info
    accel_config.info()

    # tasks that have a special config/dispatch setup
    if args.task == 'generate_hw':
       pass 

if __name__ == '__main__':
    # set the program root directory rdir to wherever chipyard is located
    # this lets you run centrifuge from anywhere, not necessarily centrifuge/deploy/
    abspath = os.path.abspath(__file__)
    dname = os.path.dirname(abspath)
    os.chdir(dname)

    global centrifuge_dir
    centrifuge_dir = up(up(abspath))
    
    global chipyard_dir
    if os.environ.get('RDIR') is None:
        chipyard_dir = up(up(up(up(abspath))))
        os.environ['RDIR'] = chipyard_dir
    else:
        chipyard_dir = os.environ.get('RDIR') 

    args = construct_centrifuge_argparser()

    # logging setup
    def logfilename():
        """ Construct a unique log file name from: date + 16 char random. """
        timeline = strftime("%Y-%m-%d--%H-%M-%S", gmtime())
        randname = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(16))
        return timeline + "-" + args.task + "-" + randname + ".log"

    rootLogger = logging.getLogger()
    rootLogger.setLevel(logging.NOTSET) # capture everything

    # log to file
    full_log_filename = "logs/" + logfilename()
    fileHandler = logging.FileHandler(full_log_filename)
    # formatting for log to file
    # TODO: filehandler should be handler 0 (firesim_topology_with_passes expects this
    # to get the filename) - handle this more appropriately later
    logFormatter = logging.Formatter("%(asctime)s [%(funcName)-12.12s] [%(levelname)-5.5s]  %(message)s")
    fileHandler.setFormatter(logFormatter)
    fileHandler.setLevel(logging.NOTSET) # log everything to file
    rootLogger.addHandler(fileHandler)

    # log to stdout, without special formatting
    consoleHandler = logging.StreamHandler(stream=sys.stdout)
    consoleHandler.setLevel(logging.INFO) # show only INFO and greater in console
    rootLogger.addHandler(consoleHandler)

    exitcode = 0
    try:
        main(args)
    except:
        # log all exceptions that make it this far
        rootLogger.exception("Fatal error.")
        exitcode = 1
    finally:
        rootLogger.info("""The full log of this run is:\n{basedir}/{fulllog}""".format(basedir=dname, fulllog=full_log_filename))
        exit(exitcode)
