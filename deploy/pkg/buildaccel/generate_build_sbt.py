import pathlib
import shutil
from .. import util
import collections
import logging
import argparse
from string import Template
import re

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG) 


def generate_build_sbt(accel_conf):
    logger.info("Generating build.sbt file ...") 

    accel_template = Template("""
lazy val ${ACCEL_NAME} = conditionalDependsOn(project in file("generators/${ACCEL_NAME}"))
  .dependsOn(boom, hwacha, sifive_blocks, sifive_cache, utilities${BMS})
  .settings(commonSettings)
""")

    bm_template = Template("""
lazy val ${BM} = (project in file("${DIR}"))
  .dependsOn(rocketchip, testchipip, midasTargetUtils, icenet)
  .settings(commonSettings)
""")
    
    accel_config_str = ""
    bms_str = "" 
    for accel in accel_conf.rocc_accels + accel_conf.tl_accels:
        d = {'BM': 'hls_' + accel.name, 'DIR': accel.dir}
        bms_str += ", hls_"+ accel.name
        accel_config_str += bm_template.substitute(d) 

    d = {'ACCEL_NAME': accel_conf.accel_name, 'BMS': bms_str}
    accel_config_str += accel_template.substitute(d) 

    template_dir = util.getOpt('template-dir')
    template_name = 'build_sbt_template'
    with open (template_dir / template_name, 'r') as f:
        template_str= f.read()

    t = Template(template_str)
    config_dict = {
        'HLS_ACCEL_NAME': accel_conf.accel_name,  
        'HLS_ACCEL_CONFIG': accel_config_str
    }
    
    tl_build_sbt_str = t.substitute(config_dict)
    chipyard_dir = util.getOpt('chipyard-dir')
    sbt_path = chipyard_dir / 'build.sbt' 
    
    logger.info("\t\tGenerate build.sbt: {}".format(sbt_path))
    with open(sbt_path,'w') as f:
        f.write(tl_build_sbt_str)

