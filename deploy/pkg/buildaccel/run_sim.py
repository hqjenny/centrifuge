import subprocess
import logging
from .. import util

logger = logging.getLogger(__name__)
logger.setLevel(logging.NOTSET) 


def run_sim_cmd(accel_conf, simulator, cmd=""):
    clean_str = "";
    
    verilator_dir = util.getOpt('chipyard-dir')/ 'sims' / simulator
    cmd = "make {} CONFIG={} TOP={} debug -j16".format(cmd, accel_conf.CONFIG, accel_conf.TOP);
    try:
        cp = subprocess.run(cmd.split(), check=True, cwd=verilator_dir, 
                universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        logging.info(cp.stdout)
    except Exception as e:
        logging.error(cp.stderr)
        raise e


def run_sim(accel_conf, simulator, subtask):
    if subtask is None:
        run_sim_cmd(accel_conf, simulator, 'clean')
        run_sim_cmd(accel_conf, simulator, 'debug')
    else:
        if subtask == 'clean':
            run_sim_cmd(accel_conf, simulator, 'clean')
        elif subtask == 'debug':
            run_sim_cmd(accel_conf, simulator, 'debug')
        else:
            raise NotImplementedError()


def run_vcs(accel_conf, subtask):
    run_sim(accel_conf, 'vcs', subtask)


def run_verilator(accel_conf, subtask):
    run_sim(accel_conf, 'verilator', subtask)


def run_firesim(accel_conf):
    pass
