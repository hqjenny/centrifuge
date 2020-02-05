from __future__ import print_function
import re

import random
import logging
import json
import pathlib

rootLogger = logging.getLogger()

class Accel(object): 
    """Base Definition of a single accelerator. All path-like objects are pathlib.Path."""
    def __init__(self, prefix_id, pgm, func, src_dir, accel_dir):
        self.prefix_id = prefix_id
        self.pgm = pgm
        self.func = func
        self.src_dir = src_dir
        self.name = """{}_{}_{}""".format(self.prefix_id, self.pgm, self.func)
        self.dir = accel_dir / self.name

        src_main_path = self.dir / 'src' / 'main'
        self.c_dir = src_main_path / 'c'
        self.verilog_dir = src_main_path / 'verilog'
        self.scala_dir = src_main_path / 'scala'

    def info(self):
        rootLogger.info(str(self))

    def __str__(self):
        return """\t\taccel_name: {} src_dir: {}""".format(self.name, self.src_dir)


class RoCCAccel(Accel): 
    """Definition of RoCC Accelerator"""
    def __init__(self, prefix_id, pgm, func, rocc_insn_id, src_dir, accel_dir):
        super(RoCCAccel, self).__init__(prefix_id, pgm, func, src_dir, accel_dir)
        self.rocc_insn_id = rocc_insn_id
    def info(self):
        rootLogger.info("""\tRoCC Accelerator {}""".format(self.prefix_id))
        super(RoCCAccel, self).info()


class TLAccel(Accel): 
    """Definition of TL Accelerator"""
    def __init__(self, prefix_id, pgm, func, base_addr, src_dir, accel_dir):
        super(TLAccel, self).__init__(prefix_id, pgm, func, src_dir, accel_dir)
        self.base_addr = base_addr
    def info(self):
        rootLogger.info("""\tTL Accelerator {} @ {}""".format(self.prefix_id, self.base_addr))
        super(TLAccel, self).info()


class AccelConfig:
    """Configuration for all accelerators to be included in the SoC
    
       Args:
            accel_json_path: pathlib.Path pointing to the config file for this accelerator
            chipyard_dir: pathlib.Path pointing to the chipyard repo to use
            centrifugre_dir: pathlib.Path pointing to the base directory of centrifuge
    """
    def __init__(self, accel_json_path, chipyard_dir, centrifuge_dir):
        self.accel_json_path = accel_json_path
        self.chipyard_dir = chipyard_dir
        self.centrifuge_dir = centrifuge_dir

        self.accel_name = self.accel_json_path.stem
        self.accel_dir = self.chipyard_dir / 'generators' / self.accel_name
        self.accel_json = self.parse_json(self.accel_json_path)

        # Lists of Accel representing the included rocc and tl accelerators (respectively)
        self.rocc_accels = []
        self.tl_accels = []

        self.parse_json_config(self.accel_json)

    def __str__(self):
        s = """Accelerator SoC Definition: \n"""
        s += "Generated SoC Directory: {}\n".format(self.accel_dir)
        for rocc_accel in self.rocc_accels:
            s += str(rocc_accel) + "\n"
        for tl_accel in self.tl_accels:
            s += str(tl_accel) + "\n"
        return s

    def info(self):
        rootLogger.info(str(self))

    def get_default_src_dir(self, pgm): 
        """
        Return the default src_dir if src_dir is not defined in the json file
        Assume the pgm is the same as the dir name under examples dir 
        """
        return self.centrifuge_dir / 'examples' / pgm

    def parse_json(self, accel_json_path):
        """Parse the JSON input"""
        rootLogger.info("Parsing input JSON file: {}".format(accel_json_path))
        with open(accel_json_path, "r") as json_file:
            return json.load(json_file)

    def check_src_dir(self, src_dir):
        if not src_dir.is_dir() and src_dir.exists():
            raise Exception("Not valid src_dir in accelerator def: {}".format(src_dir))

    def check_str(self, input_str):
        """Throw exception if str is not valid"""
        if (input_str == '' or input_str.isspace() or 
            bool(re.match('^\s+$', input_str))):
            raise Exception("Empty string is used in accelerator def: {}".format(input_str))

    def check_addr_str(self, input_str):
        """Throw exception if str is not addr"""
        try:
            int(input_str)
        except ValueError:
            try: 
                int(input_str, 16)
            except ValueError:
                try:
                    int(input_str, 2)
                except ValueError:
                    raise Exception("Invalid addr is used in accelerator def: {}".format(input_str))
        
    def parse_json_config(self, accel_json):
        """Parse the JSON config to get accel definitions """

        rootLogger.info("Parsing input JSON config")

        # Parse RoCC Accelerators 
        if 'RoCC' in accel_json.keys():
            rocc_accel_def = accel_json['RoCC']
            idx = 0
            for idx in range(3):
                try: 
                    if 'custom'+str(idx) in rocc_accel_def.keys():
                        rocc_accel_dict = rocc_accel_def['custom'+str(idx)] 
                        prefix_id = 'rocc'+str(idx)
                        pgm = rocc_accel_dict['pgm']
                        func = rocc_accel_dict['func']
                        self.check_str(pgm)
                        self.check_str(func)

                        src_dir = rocc_accel_dict.get('src_dir')
                        if src_dir is None:
                            src_dir = self.get_default_src_dir(pgm) 
                            rootLogger.info("Use default src path: {src_dir} for rocc{idx} accelerator""".format(src_dir=src_dir,idx=idx))
                        else:
                            src_dir = pathlib.Path(src_dir)
                            self.check_src_dir(src_dir)

                        rocc_accel = RoCCAccel(prefix_id, pgm, func, idx, src_dir, self.accel_dir)
                        self.rocc_accels.append(rocc_accel)
                except Exception as err:
                    rootLogger.exception(err)
                    rootLogger.exception("""Fatal error. RoCC Accelerator definitions in {} is not valid """.format(self.accel_json_path))
                    assert(False)

            if 'custom3' in rocc_accel_def.keys():
                rootLogger.exception("""Fatal error. custom3 RoCC Accelerator is reserved for Virtual-to-Physical Address Translator""")
                    
        # Parse TL Accelerators
        if 'TL' in accel_json.keys():
            tl_accel_arr = accel_json['TL']
            idx = 0
            for tl_accel_dict in tl_accel_arr:
                try: 
                    prefix_id = 'tl'+str(idx)
                    pgm = tl_accel_dict['pgm']
                    func = tl_accel_dict['func']
                    base_addr = tl_accel_dict['addr']
                    self.check_str(pgm)
                    self.check_str(func)
                    self.check_addr_str(base_addr)

                    src_dir = rocc_accel_dict.get('src_dir')
                    if src_dir is None:
                        src_dir = self.get_default_src_dir(pgm) 
                        rootLogger.info("Use default src path: {src_dir} for tl{idx} accelerator""".format(src_dir=src_dir,idx=idx))
                    else:
                        src_dir = pathlib.Path(src_dir)
                        self.check_src_dir(src_dir)

                    tl_accel = TLAccel(prefix_id, pgm, func, base_addr, src_dir, self.accel_dir)
                    self.tl_accels.append(tl_accel)
                    idx += 1
                except Exception as err:
                    rootLogger.exception(err)
                    rootLogger.exception("""Fatal error. TL Accelerator definitions in {} is not valid """.format(self.accel_json_path))
                    assert(False)
