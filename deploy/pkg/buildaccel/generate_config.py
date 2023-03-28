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
        import_str += "// import hls_{0}.WithHLS{0}\n".format(accel.name)

    rocc_arr = []
    rocc_template = Template("""(p: Parameters) => {
            val hls_${ACCEL} = LazyModule(new HLS${ACCEL}Control(OpcodeSet.custom${IDX})(p))
            hls_${ACCEL}
        }""")

    for idx, accel in enumerate(accel_conf.rocc_accels):
        d = {'ACCEL': accel.name, 'IDX': idx}
        rocc_arr.append(rocc_template.substitute(d))
    rocc_str = ",\n        ".join(rocc_arr)

    if len(accel_conf.rocc_accels) > 0:
        rocc_str += ","

    tl_peri_str = ""
    tl_trait_str = ""
    tl_trait_imp_str = ""
    for accel in accel_conf.tl_accels:
        tl_peri_str += "    new WithHLS{} ++\n".format(accel.name)
        tl_trait_str += "  with hls_{0}.CanHavePeripheryHLS{0}AXI\n".format(accel.name)
        tl_trait_imp_str += "  with hls_{0}.CanHavePeripheryHLS{0}AXIImp\n".format(accel.name)


    template_dir = util.getOpt('template-dir')
    config_dict = {
        #'SOC_NAME': accel_conf.accel_name,
        'HLS_SOC_IMPORT': import_str,
        'ROCC_CONFIG': rocc_str,
        'TL_PERIPHERY': tl_peri_str,
        #'TL_PERIPHERY_IMP': tl_peri_imp_str,
    }

    template_path = template_dir / 'HLSConfig_scala_template'
    output_path =  accel_conf.chipyard_scala_dir / 'config' / 'HLSConfig.scala'
    util.generate_file(template_path, config_dict, output_path)
    logger.info("\t\tGenerate HLSConfig.scala: {}".format(output_path))

    config_dict = {
        'SOC_NAME': accel_conf.accel_name,
        'TL_TRAIT': tl_trait_str,
        'TL_TRAIT_IMP': tl_trait_imp_str,
    }

    template_path = template_dir / 'DigitalTop_scala_template'
    output_path =  accel_conf.chipyard_scala_dir / 'DigitalTop.scala'
    util.generate_file(template_path, config_dict, output_path)
    logger.info("\t\tGenerate DigitalTop.scala: {}".format(output_path))

    d = {}
    template_path = template_dir / 'HLSTargetConfig_scala_template'
    output_path = accel_conf.firechip_scala_dir / 'HLSTargetConfig.scala'
    util.generate_file(template_path, d, output_path)
    logger.info("\t\tGenerate HLSTargetConfig.scala: {}".format(output_path))
