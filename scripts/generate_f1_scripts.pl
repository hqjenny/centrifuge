#!/usr/bin/perl
use strict;
use warnings;
use JSON qw( decode_json );
use Cwd;
use File::Copy;

my $dir = getcwd;

sub generate_f1_scripts{

    my $rdir = $ENV{'RDIR'};
    if ((not defined($rdir)) or $rdir eq '') {
        print("Please source sourceme-f1-manager.sh!\n");
        exit();
    }

    my $cl_dir = $ENV{'CL_DIR'};
    #print $cl_dir;
    if ((not defined($cl_dir)) or $cl_dir eq '') {
        print("Please source sourceme-f1.sh!\n");
        exit();
    }

    # hash of all hls bm and its path
    my %bm_path = %{$_[0]};
 
open EN_TCL, ">$cl_dir/build/scripts/encrypt.tcl";
open SYN_TCL, ">$cl_dir/build/scripts/synth_cl_firesim.tcl";

#my $bm_path = $rdir."/sim/target-rtl/firechip/hls_$file_name"."_$func_name";

my $encrypt_tcl = '# TODO:
# Add check if CL_DIR and HDK_SHELL_DIR directories exist
# Add check if /build and /build/src_port_encryption directories exist
# Add check if the vivado_keyfile exist

set HDK_SHELL_DIR $::env(HDK_SHELL_DIR)
set HDK_SHELL_DESIGN_DIR $::env(HDK_SHELL_DESIGN_DIR)
set CL_DIR $::env(CL_DIR)

set TARGET_DIR $CL_DIR/build/src_post_encryption
set UNUSED_TEMPLATES_DIR $HDK_SHELL_DESIGN_DIR/interfaces


# Remove any previously encrypted files, that may no longer be used
if {[llength [glob -nocomplain -dir $TARGET_DIR *]] != 0} {
  eval file delete -force [glob $TARGET_DIR/*]
}

#---- Developr would replace this section with design files ----

## Change file names and paths below to reflect your CL area.  DO NOT include AWS RTL files.
file copy -force $CL_DIR/design/cl_firesim_defines.vh                 $TARGET_DIR
file copy -force $CL_DIR/design/cl_firesim_generated_defines.vh       $TARGET_DIR
file copy -force $CL_DIR/design/ila_files/firesim_ila_insert_inst.v   $TARGET_DIR
file copy -force $CL_DIR/design/ila_files/firesim_ila_insert_ports.v  $TARGET_DIR
file copy -force $CL_DIR/design/ila_files/firesim_ila_insert_wires.v  $TARGET_DIR
file copy -force $CL_DIR/design/cl_id_defines.vh                      $TARGET_DIR
file copy -force $CL_DIR/design/cl_firesim.sv                         $TARGET_DIR 
file copy -force $CL_DIR/design/cl_firesim_generated.sv               $TARGET_DIR 
file copy -force $CL_DIR/../common/design/cl_common_defines.vh        $TARGET_DIR 
file copy -force $UNUSED_TEMPLATES_DIR/unused_apppf_irq_template.inc  $TARGET_DIR
file copy -force $UNUSED_TEMPLATES_DIR/unused_cl_sda_template.inc     $TARGET_DIR
file copy -force $UNUSED_TEMPLATES_DIR/unused_ddr_a_b_d_template.inc  $TARGET_DIR
# we will use ddr_c unlike hello world:
#file copy -force $UNUSED_TEMPLATES_DIR/unused_ddr_c_template.inc      $TARGET_DIR
file copy -force $UNUSED_TEMPLATES_DIR/unused_dma_pcis_template.inc   $TARGET_DIR
file copy -force $UNUSED_TEMPLATES_DIR/unused_pcim_template.inc       $TARGET_DIR
file copy -force $UNUSED_TEMPLATES_DIR/unused_sh_bar1_template.inc    $TARGET_DIR
file copy -force $UNUSED_TEMPLATES_DIR/unused_flr_template.inc        $TARGET_DIR

';

while(my($bm, $path) = each %bm_path) {

system("mkdir -p $cl_dir/design/$bm");
system("cp $path/src/main/verilog/* $cl_dir/design/$bm");
#my $copy_hls_src = "set src_list [glob $path/src/main/verilog/*]\n".'foreach src $src_list {
my $copy_hls_src = "set src_list [glob \$CL_DIR/design/$bm/*]\n".'foreach src $src_list {
    file copy -force $src $TARGET_DIR
}
';
#print($copy_hls_src);
$encrypt_tcl .= $copy_hls_src;
}

$encrypt_tcl .= '# Make sure files have write permissions for the encryption

exec chmod +w {*}[glob $TARGET_DIR/*]

set TOOL_VERSION $::env(VIVADO_TOOL_VERSION)
set vivado_version [string range [version -short] 0 5]
puts "AWS FPGA: VIVADO_TOOL_VERSION $TOOL_VERSION"
puts "vivado_version $vivado_version"

# encrypt .v/.sv/.vh/inc as verilog files
encrypt -k $HDK_SHELL_DIR/build/scripts/vivado_keyfile_2017_4.txt -lang verilog  [glob -nocomplain -- $TARGET_DIR/*.{v,sv}] [glob -nocomplain -- $TARGET_DIR/*.vh] [glob -nocomplain -- $TARGET_DIR/*.inc]
# encrypt *vhdl files
encrypt -k $HDK_SHELL_DIR/build/scripts/vivado_vhdl_keyfile_2017_4.txt -lang vhdl -quiet [ glob -nocomplain -- $TARGET_DIR/*.vhd? ]
';

print EN_TCL $encrypt_tcl;
close EN_TCL;

my @tcl_files =();
# Find the HLS generated IP tcl to source
while(my($bm, $path) = each %bm_path) {
    #chdir($path.'/src/main/verilog') or die "$!";
    chdir("$cl_dir/design/$bm") or die "$!";
    #my @local_tcl_files = map { Cwd::abs_path($_) } glob "*.tcl";
    my @local_tcl_files = map { "\$CL_DIR/design/$bm/".$_ } glob "*.tcl";
    push(@tcl_files, @local_tcl_files);
}

my $synth_cl_firesim = '
########################################
## Generate ILA based on Recipe 
########################################

puts "AWS FPGA: ([clock format [clock seconds] -format %T]) Calling firesim_ila_insert_vivado.tcl to generate ILAs from developer\'s specified recipe.";

source $CL_DIR/design/ila_files/firesim_ila_insert_vivado.tcl

#Param needed to avoid clock name collisions
set_param sta.enableAutoGenClkNamePersistence 0
set CL_MODULE $CL_MODULE
set VDEFINES $VDEFINES

create_project -in_memory -part [DEVICE_TYPE] -force

########################################
## Generate clocks based on Recipe 
########################################

puts "AWS FPGA: ([clock format [clock seconds] -format %T]) Calling aws_gen_clk_constraints.tcl to generate clock constraints from developer\'s specified recipe.";
';

foreach my $tcl_file (@tcl_files) {
    $synth_cl_firesim .="source $tcl_file\n";
}

$synth_cl_firesim .= 'source $HDK_SHELL_DIR/build/scripts/aws_gen_clk_constraints.tcl

#############################
## Read design files
#############################

#Convenience to set the root of the RTL directory
set ENC_SRC_DIR $CL_DIR/build/src_post_encryption

puts "AWS FPGA: ([clock format [clock seconds] -format %T]) Reading developer\'s Custom Logic files post encryption.";

#---- User would replace this section -----

# Reading the .sv and .v files, as proper designs would not require
# reading .v, .vh, nor .inc files

read_verilog -sv [glob $ENC_SRC_DIR/*.?v]
read_verilog -sv [glob $ENC_SRC_DIR/*.v]
set v_list  [glob $ENC_SRC_DIR/*.v]
puts $v_list

#---- End of section replaced by User ----

puts "AWS FPGA: Reading AWS Shell design";

#Read AWS Design files
read_verilog -sv [ list \
  $HDK_SHELL_DESIGN_DIR/lib/lib_pipe.sv \
  $HDK_SHELL_DESIGN_DIR/sh_ddr/synth/sync.v \
  $HDK_SHELL_DESIGN_DIR/sh_ddr/synth/flop_ccf.sv \
  $HDK_SHELL_DESIGN_DIR/sh_ddr/synth/ccf_ctl.v \
  $HDK_SHELL_DESIGN_DIR/sh_ddr/synth/sh_ddr.sv \
  $HDK_SHELL_DESIGN_DIR/lib/lib_pipe.sv \
  $HDK_SHELL_DESIGN_DIR/lib/bram_2rw.sv \
  $HDK_SHELL_DESIGN_DIR/lib/flop_fifo.sv \
  $HDK_SHELL_DESIGN_DIR/sh_ddr/synth/mgt_acc_axl.sv  \
  $HDK_SHELL_DESIGN_DIR/sh_ddr/synth/mgt_gen_axl.sv  \
  $HDK_SHELL_DESIGN_DIR/interfaces/cl_ports.vh
]

puts "AWS FPGA: Reading IP blocks";

#Read IP for axi register slices
read_ip [ list \
  $HDK_SHELL_DESIGN_DIR/ip/src_register_slice/src_register_slice.xci \
  $HDK_SHELL_DESIGN_DIR/ip/dest_register_slice/dest_register_slice.xci \
  $HDK_SHELL_DESIGN_DIR/ip/axi_register_slice/axi_register_slice.xci \
  $HDK_SHELL_DESIGN_DIR/ip/axi_register_slice_light/axi_register_slice_light.xci
]

#Read IP for virtual jtag / ILA/VIO
read_ip [ list \
  $HDK_SHELL_DESIGN_DIR/ip/ila_0/ila_0.xci\
  $HDK_SHELL_DESIGN_DIR/ip/cl_debug_bridge/cl_debug_bridge.xci \
  $HDK_SHELL_DESIGN_DIR/ip/ila_vio_counter/ila_vio_counter.xci \
  $HDK_SHELL_DESIGN_DIR/ip/vio_0/vio_0.xci \
  $HDK_SHELL_DESIGN_DIR/ip/axi_clock_converter_0/axi_clock_converter_0.xci \
  $CL_DIR/ip/axi_clock_converter_dramslim/axi_clock_converter_dramslim.xci \
  $CL_DIR/ip/axi_clock_converter_oclnew/axi_clock_converter_oclnew.xci \
  $CL_DIR/ip/axi_clock_converter_512_wide/axi_clock_converter_512_wide.xci \
  $CL_DIR/ip/axi_dwidth_converter_0/axi_dwidth_converter_0.xci \
  $CL_DIR/ip/firesim_ila_ip/ila_firesim_0/ila_firesim_0.xci
]

# Additional IPs that might be needed if using the DDR
read_ip [ list \
 $HDK_SHELL_DESIGN_DIR/ip/ddr4_core/ddr4_core.xci
]
read_bd [ list \
 $HDK_SHELL_DESIGN_DIR/ip/cl_axi_interconnect/cl_axi_interconnect.bd
]

puts "AWS FPGA: Reading AWS constraints";

#Read all the constraints
#
#  cl_clocks_aws.xdc  - AWS auto-generated clock constraint.   ***DO NOT MODIFY***
#  cl_ddr.xdc         - AWS provided DDR pin constraints.      ***DO NOT MODIFY***
#  cl_synth_user.xdc  - Developer synthesis constraints.
read_xdc [ list \
   $CL_DIR/build/constraints/cl_clocks_aws.xdc \
   $HDK_SHELL_DIR/build/constraints/cl_ddr.xdc \
   $HDK_SHELL_DIR/build/constraints/cl_synth_aws.xdc \
   $CL_DIR/build/constraints/cl_synth_user.xdc
]

# FireSim custom clocking
source $CL_DIR/build/scripts/synth_firesim_clk_wiz.tcl

#Do not propagate local clock constraints for clocks generated in the SH
set_property USED_IN {synthesis implementation OUT_OF_CONTEXT} [get_files cl_clocks_aws.xdc]
set_property PROCESSING_ORDER EARLY  [get_files cl_clocks_aws.xdc]

########################
# CL Synthesis
########################
puts "AWS FPGA: ([clock format [clock seconds] -format %T]) Start design synthesis.";

update_compile_order -fileset sources_1
puts "\nRunning synth_design for $CL_MODULE $CL_DIR/build/scripts \[[clock format [clock seconds] -format {%a %b %d %H:%M:%S %Y}]\]"
eval [concat synth_design -top $CL_MODULE -verilog_define XSDB_SLV_DIS $VDEFINES -part [DEVICE_TYPE] -mode out_of_context $synth_options -directive $synth_directive -retiming]

set failval [catch {exec grep "FAIL" failfast.csv}]
if { $failval==0 } {
	puts "AWS FPGA: FATAL ERROR--Resource utilization error; check failfast.csv for details"
	exit 1
}

puts "AWS FPGA: ([clock format [clock seconds] -format %T]) writing post synth checkpoint.";
write_checkpoint -force $CL_DIR/build/checkpoints/${timestamp}.CL.post_synth.dcp

close_project
#Set param back to default value
set_param sta.enableAutoGenClkNamePersistence 1
';

print SYN_TCL $synth_cl_firesim;
close SYN_TCL;

}

1;
