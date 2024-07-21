import shutil
import os
import pathlib
import errno
import fileinput
import subprocess
import sys
from string import Template


def exec_cmd(cmd_arr, cwd, logger):
    # Use Popen for backward compatibility 
    proc = subprocess.Popen(cmd_arr, cwd=cwd, 
            universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    outs, errs = proc.communicate()
    returncode = proc.returncode
    if returncode is not 0: 
        logger.error(errs)
        raise Exception
    else: 
        logger.info(outs)


def append_to_file(src_file_path, dest_file_path):
    with open(src_file_path, "r") as f:
        src_data = f.read()
    with open(dest_file_path, "a") as f:
        f.write(src_data)


def replace_str(file_path, pattern, subst):
    for line in fileinput.input(str(file_path), inplace=True):
        if pattern in line:
            line = line.replace(pattern, subst)
        sys.stdout.write(line)


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


def generate_file(template_path, config_dict, output_path):
    with open (template_path, 'r') as f:
        template_str = f.read()

    t = Template(template_str)
    output_str = t.substitute(config_dict)
    with open(output_path, 'w') as f:
        f.write(output_str)

