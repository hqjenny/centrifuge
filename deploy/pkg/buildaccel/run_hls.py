from __future__ import print_function
import re

import random
import logging
import json
import os
import shutil

import errno    

from string import Template

rootLogger = logging.getLogger()

def run_hls_cmd():
    pass


def read_template(path):
    with open(path) as f:
        src = Template(f.read())
    return src

def run_hls(accel):
    pass

    
