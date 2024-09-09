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


def generate_build_sbt(accel_conf):
    logger.info("Generating build.sbt file ...")

    accel_template = Template("""
lazy val ${SOC_NAME} = (project in file("generators/${SOC_NAME}"))
  .dependsOn(boom, testchipip, rocketchip_blocks, rocketchip_inclusive_cache${ACCELS})
  .settings(libraryDependencies ++= rocketLibDeps.value)
  .settings(commonSettings)
""")

    bm_template = Template("""
lazy val ${ACCEL} = (project in file("${DIR}"))
  .dependsOn(rocketchip, testchipip, icenet)
  .settings(libraryDependencies ++= rocketLibDeps.value)
  .settings(commonSettings)
""")

    accel_config_str = ""
    bms_str = ""
    for accel in accel_conf.rocc_accels + accel_conf.tl_accels:
        d = {'ACCEL': 'hls_' + accel.name, 'DIR': accel.dir}
        bms_str += ", hls_"+ accel.name
        accel_config_str += bm_template.substitute(d)

    d = {'SOC_NAME': accel_conf.accel_name, 'ACCELS': bms_str}
    accel_config_str += accel_template.substitute(d)

    template_path = util.getOpt('template-dir') / 'build_sbt_template'
    hls_soc_name_str = accel_conf.accel_name
    config_dict = {
        'HLS_SOC_NAME': hls_soc_name_str,
        'HLS_SOC_CONFIG': accel_config_str
    }
    chipyard_dir = util.getOpt('chipyard-dir')
    sbt_path = chipyard_dir / 'build.sbt'
    util.generate_file(template_path, config_dict, sbt_path)
    logger.info("\t\tGenerate build.sbt: {}".format(sbt_path))

