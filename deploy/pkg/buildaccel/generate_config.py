import pathlib
import shutil
import collections
import logging
import argparse
from string import Template
import re
from .. import util

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG) 

def generate_config(accel_conf):
    logger.info("Generating HLSConfig.scala file ...") 
    logger.info("Generating HLSFireSimConfig.scala file ...") 

    import_str = ""
    for accel in accel_conf.rocc_accels:
        import_str += "import hls_{0}.HLS{0}Control\n".format(accel.name)
    for accel in accel_conf.tl_accels:
        import_str += "import hls_{0}._\n".format(accel.name)

    rocc_arr = []
    rocc_template = Template("""(p: Parameters) => {
            val hls_${ACCEL} = LazyModule(new HLS${ACCEL}Control(OpcodeSet.custom${IDX})(p))
            hls_${ACCEL}
        }""")

    for idx, accel in enumerate(accel_conf.rocc_accels):
        d = {'ACCEL': 'hls_' + accel.name, 'IDX': idx}
        rocc_arr.append(rocc_template.substitute(d)) 
    rocc_str = ",\n        ".join(rocc_arr)

    if len(accel_conf.rocc_accels) > 0: 
        rocc_str += "," 

    tl_peri_str = "" 
    tl_peri_imp_str = ""
    for accel in accel_conf.tl_accels:
        tl_peri_str += "\n    with HasPeripheryHLS{}AXI".format(accel.name)
        tl_peri_imp_str += "\n    with HasPeripheryHLS{}AXIImp".format(accel.name)

    template_dir = util.getOpt('template-dir')
    config_dict = {
        'SOC_NAME': accel_conf.accel_name,
        'HLS_SOC_IMPORT': import_str,  
        'ROCC_CONFIG': rocc_str,
        'TL_PERIPHERY': tl_peri_str,
        'TL_PERIPHERY_IMP': tl_peri_imp_str,
    }
    
    template_path = template_dir / 'HLSConfig_scala_template'
    output_path =  accel_conf.hw_scala_dir / 'HLSConfig.scala'
    util.generate_file(template_path, config_dict, output_path)
    logger.info("\t\tGenerate HLSConfig.scala: {}".format(output_path))

    template_path = template_dir / 'HLSFireSimConfig_scala_template'
    output_path =  accel_conf.hw_scala_dir / 'HLSFireSimConfig.scala'
    util.generate_file(template_path, config_dict, output_path)
    logger.info("\t\tGenerate HLSFireSimConfig.scala: {}".format(output_path))

