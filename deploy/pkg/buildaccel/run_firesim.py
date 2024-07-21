import os
import subprocess
import logging
import pathlib
from string import Template
from .. import util

logger = logging.getLogger(__name__)
logger.setLevel(logging.NOTSET) 

CONFIG_WITH_NIC = 'firesim-rocket-singlecore-hls-nic-l2-llc4mb-ddr3'
CONFIG_NO_NIC = 'firesim-rocket-singlecore-hls-no-nic-l2-llc4mb-ddr3'

def get_env_var():
    CL_DIR = os.getenv('CL_DIR')
    if CL_DIR is None:  
        logging.error("Please source sourceme-f1-manager.sh!")
        raise
    CL_DIR = pathlib.Path(CL_DIR)
    return CL_DIR


def generate_encrypt_tcl(accel_conf, CL_DIR):

    t = Template("""set src_list [glob ${VERILOG_DIR}/*]
foreach src $$src_list {
    file copy -force $$src $$TARGET_DIR
}
""")

    hls_srcs_str = "" 
    for accel in accel_conf.rocc_accels + accel_conf.tl_accels:
        src_dir = accel.verilog_dir
        dst_dir = CL_DIR / 'design' / accel_conf.accel_name / accel.name
        dst_dir_str = "$CL_DIR/design/{}/{}".format(accel_conf.accel_name, accel.name)
        util.mkdir_p(dst_dir)
        util.copytree(src_dir, dst_dir)

        d = {'VERILOG_DIR': dst_dir_str}
        hls_srcs_str += t.substitute(d) 
        #hls_srcs_str += """file mkdir $TARGET_DIR/{0}/{1}
#file copy -force $CL_DIR/design/{0}/{1} $TARGET_DIR/{0}/{1}
#""".format(accel_conf.accel_name, accel.name)

    template_path = util.getOpt('template-dir') / 'encrypt_tcl_template'
    d = {
        'COPY_HLS_SRCS': hls_srcs_str,  
    }
    output_path = CL_DIR / 'build' / 'scripts' / 'encrypt.tcl'
    util.generate_file(template_path, d, output_path)
    logger.info("\t\tGenerate encrypt.tcl: {}".format(output_path))


def generate_synth_cl_firesim_tcl(accel_conf, CL_DIR):

    tcl_src_paths = []
    for accel in accel_conf.rocc_accels + accel_conf.tl_accels:
        cl_dir = CL_DIR / 'design' / accel_conf.accel_name / accel.name
        design_verilog_dir = cl_dir
        # tcl_srcs = list(design_verilog_dir.glob('**/*.tcl'))
        tcl_srcs = list(design_verilog_dir.glob('*.tcl'))
        for tcl_src in tcl_srcs: 
            tcl_src_path = "$CL_DIR/design/{}/{}/{}".format(accel_conf.accel_name, accel.name, tcl_src.name)
            tcl_src_paths.append(tcl_src_path)
        
    src_tcl_str = ""    
    for tcl_path in tcl_src_paths:
        hls_srcs_str += "source {}".format(tcl_path)

    template_path = util.getOpt('template-dir') / 'synth_cl_firesim_tcl_template'
    d = {
        'SOURCE_TCL_FILES': src_tcl_str,  
    }
    output_path = CL_DIR / 'build' / 'scripts' / 'synth_cl_firesim.tcl'
    util.generate_file(template_path, d, output_path)
    logger.info("\t\tGenerate synth_cl_firesim.tcl: {}".format(output_path))


def generate_f1_scripts(accel_conf):
    logger.info("Generating F1 Scripts ...") 
    CL_DIR = get_env_var()
    generate_synth_cl_firesim_tcl(accel_conf, CL_DIR)
    generate_encrypt_tcl(accel_conf, CL_DIR)


def generate_firesim_build_recipes(accel_conf):
    logger.info("Generating cf_config_build_recipes.ini file ...") 

    template_path = util.getOpt('template-dir') / 'cf_config_build_recipes_ini_template'
    d = {}
    config_path = accel_conf.hw_accel_dir / 'cf_config_build_recipes.ini' 
    util.generate_file(template_path, d, config_path)
    logger.info("\t\tGenerate cf_config_build_recipes.ini: {}".format(config_path))


def generate_firesim_build(accel_conf, with_nic, s3_bucket):
    logger.info("Generating cf_config_build.ini file ...") 

    template_path = util.getOpt('template-dir') / 'cf_config_build_ini_template'

    if with_nic:
       recipe_str = CONFIG_WITH_NIC 
    else:
       recipe_str = CONFIG_NO_NIC 
    d = {
        'S3_BUCKET_NAME': s3_bucket,
        'RECIPE': recipe_str
        }
    config_path = accel_conf.hw_accel_dir / 'cf_config_build.ini' 
    util.generate_file(template_path, d, config_path)
    logger.info("\t\tGenerate cf_config_build.ini: {}".format(config_path))


def generate_firesim_hwdb(accel_conf, with_nic, agfi):
    logger.info("Generating cf_config_hwdb.ini file ...") 

    template_path = util.getOpt('template-dir') / 'cf_config_hwdb_ini_template'

    if with_nic:
       recipe_str = CONFIG_WITH_NIC 
    else:
       recipe_str = CONFIG_NO_NIC 
    d = {
        'RECIPE': recipe_str,
        'AGFI': agfi,
        }
    config_path = accel_conf.hw_accel_dir / 'cf_config_hwdb.ini' 
    util.generate_file(template_path, d, config_path)
    logger.info("\t\tGenerate cf_config_hwdb.ini: {}".format(config_path))


def generate_firesim_runtime(accel_conf, with_nic, workload='linux-uniform.json'):
    logger.info("Generating cf_config_runtime.ini file ...") 

    template_path = util.getOpt('template-dir') / 'cf_config_runtime_ini_template'

    if with_nic:
       recipe_str = CONFIG_WITH_NIC 
    else:
       recipe_str = CONFIG_NO_NIC 
    d = {
        'RECIPE': recipe_str,
        'WORKLOAD': workload,
        }
    config_path = accel_conf.hw_accel_dir / 'cf_config_runtime.ini' 
    util.generate_file(template_path, d, config_path)
    logger.info("\t\tGenerate cf_config_runtime.ini: {}".format(config_path))


def run_firesim_task(accel_conf, firesim_task):
    config_files = ["build", "build_recipes", "hwdb", "runtime"]
    cmd_args = ""

    for config in config_files:
        config_path = accel_conf.hw_accel_dir / "cf_config_{}.ini".format(config)
        cmd_args += " --{}configfile {} ".format(config.replace('_',''), str(config_path))

    cmd = "firesim {} {}".format(firesim_task, cmd_args)
    logger.info("\t\tCommand: {}".format(cmd))

    cmd_arr = cmd.split()
    subprocess.check_call(cmd_arr)


def run_firesim(accel_conf, args):
    subtask = args.subtask
    if subtask is None:
        generate_f1_scripts(accel_conf)
        generate_firesim_build_recipes(accel_conf)
        generate_firesim_build(accel_conf, args.with_nic, args.s3_bucket)
        generate_firesim_hwdb(accel_conf, args.with_nic, args.agfi)
        generate_firesim_runtime(accel_conf, args.with_nic, args.workload)
    else:
        if subtask == "f1_scripts":
            generate_f1_scripts(accel_conf)
        elif subtask == "build_recipes":
            generate_firesim_build_recipes(accel_conf)
        elif subtask == "build":
            generate_firesim_build(accel_conf, args.with_nic, args.s3_bucket)
        elif subtask == "hwdb":
            generate_firesim_hwdb(accel_conf, args.with_nic, args.agfi)
        elif subtask == "runtime":
            generate_firesim_runtime(accel_conf, args.with_nic, args.workload)
        elif subtask == "task":
            run_firesim_task(accel_conf, args.firesim_task)
        else:
            raise NotImplementedError()


   
