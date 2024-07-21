from __future__ import print_function
import re

import random
import logging
import json
import pathlib

rootLogger = logging.getLogger()

class Accel(object):
    """Base Definition of Accelerator"""
    def __init__(self, prefix_id, pgm, func, srcs, hw_accel_dir):
        self.prefix_id = prefix_id
        self.pgm = pgm
        self.func = func
        self.srcs = srcs
        self.name = """{}_{}_{}""".format(self.prefix_id, self.pgm, self.func)
        self.dir = hw_accel_dir / self.name

        src_main_path = self.dir / 'src' / 'main'
        self.c_dir = src_main_path / 'c'
        #self.verilog_dir = src_main_path / 'resources' / 'vsrc'
        self.verilog_dir = src_main_path / 'verilog'
        self.scala_dir = src_main_path / 'scala'

    def info(self):
        rootLogger.info(str(self))

    def __str__(self):
        return """\t\taccel_name: {} srcs: {}""".format(self.name, self.srcs)


class RoCCAccel(Accel):
    """Definition of RoCC Accelerator"""
    def __init__(self, prefix_id, pgm, func, rocc_insn_id, src_dir, hw_accel_dir):
        super(RoCCAccel, self).__init__(prefix_id, pgm, func, src_dir, hw_accel_dir)
        self.rocc_insn_id = rocc_insn_id
    def info(self):
        rootLogger.info("""\tRoCC Accelerator {}""".format(self.prefix_id))
        super(RoCCAccel, self).info()


class TLAccel(Accel):
    """Definition of TL Accelerator"""
    def __init__(self, prefix_id, pgm, func, base_addr, src_dir, hw_accel_dir):
        super(TLAccel, self).__init__(prefix_id, pgm, func, src_dir, hw_accel_dir)
        self.base_addr = base_addr
    def info(self):
        rootLogger.info("""\tTL Accelerator {} @ {}""".format(self.prefix_id, self.base_addr))
        super(TLAccel, self).info()


class AccelConfig:
    """Configuration for all accelerators to be included in the SoC"""

    def __init__(self, accel_json_path, chipyard_dir, centrifuge_dir, genhw_dir):
        self.accel_json_path = accel_json_path
        self.chipyard_dir = chipyard_dir
        self.centrifuge_dir = centrifuge_dir

        self.sims_dir = self.chipyard_dir / 'sims'
        self.chipyard_scala_dir = self.chipyard_dir / 'generators' / 'chipyard' / 'src' / 'main' / 'scala'
        self.firechip_scala_dir = self.chipyard_dir / 'generators' / 'firechip' / 'src' / 'main' / 'scala'
        self.accel_name = self.accel_json_path.stem
        self.accel_json_dir = accel_json_path.parent
        self.gensw_dir = self.accel_json_dir / 'centrifuge_wrappers'
        self.hw_accel_dir = genhw_dir / self.accel_name
        self.hw_accel_scala_dir = self.hw_accel_dir / 'src' / 'main' / 'scala'
        self.accel_json = self.parse_json(self.accel_json_path)
        self.rocc_accels = []
        self.tl_accels = []
        self.parse_json_config(self.accel_json)

        # NIC
        nonic = 'NoNIC'

        # Default config string
        self.DESIGN= self.accel_name + 'FireSimTopWithHLS' + nonic
        self.TARGET_CONFIG = self.accel_name + 'HLSFireSimRocketChipConfig'
        self.PLATFORM_CONFIG='BaseF1Config_F90MHz'

        self.CONFIG = 'HLSRocketConfig'
        self.TOP = 'DigitalTop'


    def __str__(self):
        s = """Accelerator SoC Definition: \n"""
        s += "Generated SoC Directory: {}\n".format(self.hw_accel_dir)
        for rocc_accel in self.rocc_accels:
            s += str(rocc_accel) + "\n"
        for tl_accel in self.tl_accels:
            s += str(tl_accel) + "\n"
        return s

    def info(self):
        rootLogger.info(str(self))

    def parse_json(self, accel_json_path):
        """Parse the JSON input"""
        rootLogger.info("Parsing input JSON file: {}".format(accel_json_path))
        with open(accel_json_path, "r") as json_file:
            return json.load(json_file)

    def check_src_path(self, src_path):
        if not (src_path.exists()):
            raise Exception("{} does not exist in accel 'srcs' def".format(src_path))

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

    def update_if_def(self, key, accel_json):
        if key in list(accel_json.keys()):
            setattr(self, key, accel_json[key])

    def parse_json_config(self, accel_json):
        """Parse the JSON config to get accel definitions """

        rootLogger.info("Parsing input JSON config")

        # Parse Config Strings
        self.update_if_def('DESIGN', accel_json);
        self.update_if_def('TARGET_CONFIG', accel_json);
        self.update_if_def('PLATFORM_CONFIG', accel_json);
        self.update_if_def('CONFIG', accel_json);
        self.update_if_def('TOP', accel_json);

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

                        srcs = rocc_accel_dict.get('srcs')
                        if srcs is None:
                            raise Exception("Please specify the 'srcs' in your accelerator definition for {}!".format(prefix_id))
                        else:
                            for i, _ in enumerate(srcs):
                                srcs[i] = pathlib.Path(srcs[i])
                                if not srcs[i].is_absolute():
                                    srcs[i] = (self.accel_json_dir / srcs[i]).resolve()
                                self.check_src_path(srcs[i])

                        rocc_accel = RoCCAccel(prefix_id, pgm, func, idx, srcs, self.hw_accel_dir)
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

                    srcs = tl_accel_dict.get('srcs')
                    if srcs is None:
                        raise Exception("Please specify the 'srcs' in your accelerator definition for {}!".format(prefix_id))
                    else:
                        for i, _ in enumerate(srcs):
                            srcs[i] = pathlib.Path(srcs[i])
                            if not srcs[i].is_absolute():
                                srcs[i] = (self.accel_json_dir / srcs[i]).resolve()
                            self.check_src_path(srcs[i])

                    tl_accel = TLAccel(prefix_id, pgm, func, base_addr, srcs, self.hw_accel_dir)
                    self.tl_accels.append(tl_accel)
                    idx += 1
                except Exception as err:
                    rootLogger.exception(err)
                    rootLogger.exception("""Fatal error. TL Accelerator definitions in {} is not valid """.format(self.accel_json_path))
                    assert(False)


