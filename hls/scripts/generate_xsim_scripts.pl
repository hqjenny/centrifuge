#!/usr/bin/perl
use strict;
use warnings;
use JSON qw( decode_json );
use Cwd;
use File::Copy;
use File::Find;

sub generate_xsim_scripts{

    my $dir = getcwd;
    my $rdir = $ENV{'RDIR'};
    #$json_fn = "accel_template.json";

    if ((not defined($rdir)) or $rdir eq '') {
        print("Please source sourceme-f1-manager.sh!\n");
        exit();
    }

    # hash of all hls bm and its path
    my %bm_path = %{$_[0]};

    my $cl_dir = $ENV{'CL_DIR'};
    #print $cl_dir;
    if ((not defined($cl_dir)) or $cl_dir eq '') {
        print("Please source sourceme-f1.sh!\n");
        exit();
    }

    copy("$cl_dir/verif/scripts/top.vivado.f","$cl_dir/verif/scripts/top.vivado.f.bk") or die "Copy failed: $!";
    open TOP_VIVADO_F, ">$cl_dir/verif/scripts/top.vivado.f";

    my $ver_inputs = "-define VIVADO_SIM
-define RANDOMIZE_MEM_INIT
-define RANDOMIZE_REG_INIT
-define RANDOMIZE_GARBAGE_ASSIGN
-define RANDOMIZE_INVALID_ASSIGN
-define PRINTF_COND=1'b1
-define STOP_COND=1'b1

-sourcelibext .v
-sourcelibext .sv
-sourcelibext .svh
".'
-sourcelibdir ${CL_ROOT}/../common/design
-sourcelibdir ${CL_ROOT}/design
-sourcelibdir ${CL_ROOT}/design/ila_files
-sourcelibdir ${CL_ROOT}/verif/sv
-sourcelibdir ${SH_LIB_DIR}
-sourcelibdir ${SH_INF_DIR}
-sourcelibdir ${SH_SH_DIR}
-sourcelibdir ${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/hdl
-sourcelibdir ${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/sim
';

    $ver_inputs .='-include ${CL_ROOT}/../common/design
-include ${CL_ROOT}/verif/sv
-include ${CL_ROOT}/design/ila_files
-include ${SH_LIB_DIR}
-include ${SH_INF_DIR}
-include ${SH_SH_DIR}
-include ${HDK_COMMON_DIR}/verif/include
-include ${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_0/sim
-include ${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_0/hdl/verilog
-include ${HDK_SHELL_DESIGN_DIR}/ip/axi_register_slice/hdl
-include ${HDK_SHELL_DESIGN_DIR}/ip/axi_register_slice_light/hdl
-include ${CL_ROOT}/ip/axi_clock_converter_oclnew/hdl
-include ${CL_ROOT}/ip/axi_dwidth_converter_0/hdl

';
    $ver_inputs .='${CL_ROOT}/../common/design/cl_common_defines.vh
${CL_ROOT}/design/cl_firesim_defines.vh
${CL_ROOT}/design/ila_files/firesim_ila_insert_inst.v
${CL_ROOT}/design/ila_files/firesim_ila_insert_ports.v
${CL_ROOT}/design/ila_files/firesim_ila_insert_wires.v
${HDK_SHELL_DESIGN_DIR}/ip/ila_vio_counter/sim/ila_vio_counter.v
${HDK_SHELL_DESIGN_DIR}/ip/ila_0/sim/ila_0.v
${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/sim/bd_a493.v
${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_0/sim/bd_a493_xsdbm_0.v
${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_0/hdl/xsdbm_v3_0_vl_rfs.v
${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_0/hdl/ltlib_v1_0_vl_rfs.v
${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_1/sim/bd_a493_lut_buffer_0.v
${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_1/hdl/lut_buffer_v2_0_vl_rfs.v
${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/hdl/bd_a493_wrapper.v
${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/sim/cl_debug_bridge.v
${HDK_SHELL_DESIGN_DIR}/ip/vio_0/sim/vio_0.v
${HDK_SHELL_DESIGN_DIR}/ip/axi_register_slice_light/sim/axi_register_slice_light.v
${HDK_SHELL_DESIGN_DIR}/ip/axi_register_slice/sim/axi_register_slice.v
${HDK_SHELL_DESIGN_DIR}/ip/axi_register_slice_light/hdl/axi_register_slice_v2_1_vl_rfs.v
${HDK_SHELL_DESIGN_DIR}/ip/axi_register_slice_light/hdl/axi_infrastructure_v1_1_vl_rfs.v
${HDK_SHELL_DESIGN_DIR}/ip/axi_clock_converter_0/hdl/axi_clock_converter_v2_1_vl_rfs.v
${SH_LIB_DIR}/../ip/axi_clock_converter_0/sim/axi_clock_converter_0.v
${CL_ROOT}/ip/axi_clock_converter_dramslim/sim/axi_clock_converter_dramslim.v
${CL_ROOT}/ip/axi_clock_converter_oclnew/sim/axi_clock_converter_oclnew.v
${CL_ROOT}/ip/axi_clock_converter_oclnew/hdl/axi_clock_converter_v2_1_vl_rfs.v
${CL_ROOT}/ip/axi_clock_converter_512_wide/sim/axi_clock_converter_512_wide.v
${CL_ROOT}/ip/clk_wiz_0_firesim/clk_wiz_0_firesim_sim_netlist.v
${CL_ROOT}/ip/axi_dwidth_converter_0/sim/axi_dwidth_converter_0.v
${CL_ROOT}/ip/axi_dwidth_converter_0/hdl/axi_dwidth_converter_v2_1_vl_rfs.v
${CL_ROOT}/ip/axi_dwidth_converter_0/hdl/axi_register_slice_v2_1_vl_rfs.v
${CL_ROOT}/design/cl_firesim_generated.sv
${CL_ROOT}/design/cl_firesim.sv
';


    my @vlogs;
    my @vhdls;

while(my($bm, $path) = each %bm_path) {
    # First synthesize the IPs 
    chdir($path.'/src/main/verilog') or die "$!";

    # Check if there is tcl scripts for ips  
    # If yes, generate source code for the ips 
    my $has_tcl = 0;
    my @sim_paths;
    if(<*.tcl>) {
        $has_tcl = 1;

        #opendir(DIR, ".");
        #my @files = grep(/\.tcl$/, readdir(DIR));
        my @files = map { Cwd::abs_path($_) } glob "*.tcl";
        #closedir(DIR);

        mkdir('./prj');
        chdir('./prj');

        print "TCL Files for Generating IPs: \n";
        foreach my $file (@files) {
           print "$file\n";
        }
        open INIT, ">init_ip.tcl";
        my $init = 'set_msg_config -severity INFO -suppress
    set_msg_config -severity STATUS -suppress
    set_msg_config -severity WARNING -suppress
    set_msg_config -string {exportsim} -suppress
    set_msg_config -string {IP_Flow} -suppress

    create_project -force tmp_ips ./ips -part xcvu9p-flgb2104-2-i
    set_property target_language Verilog [current_project]
    ';
       
        foreach my $file (@files) {
            $init .="source $file\n";
        }

        $init .= "exit\n"; 
        print INIT $init;
        close INIT;

        system('vivado -mode batch -source init_ip.tcl  && echo "success" || echo "failed"');
        if ($? == -1) {
            print "failed to execute: $!\n";
        }

          find({ wanted => sub { if(basename($_) eq "sim") {push @sim_paths, Cwd::abs_path($_)} } , no_chdir => 1 }, "./ips");
    }

    # Add HLS source folder
    push (@sim_paths, "$path/src/main/verilog");
    print("Simulation File Folder: \n");
    foreach my $sim_path(@sim_paths) {
        print "$sim_path\n";
    }

    find({ wanted => sub { push @vlogs, map { Cwd::abs_path($_) } glob("\"$_/*.v\"")} , no_chdir => 1 }, @sim_paths);
    find({ wanted => sub { push @vhdls, map { Cwd::abs_path($_) } glob("\"$_/*.vhd\"")} , no_chdir => 1 }, @sim_paths);


    print("Verilog Source Files: \n");
    foreach my $vlog(@vlogs) {
       print "$vlog\n";
    }

    print("VHDL Source Files: \n");
    foreach my $vhdl(@vhdls) {
       print "$vhdl\n";
    }


    my $new_src_dir = "-sourcelibdir $path/src/main/verilog\n";
    $ver_inputs .= $new_src_dir;


    foreach my $vlog(@vlogs) {
        $ver_inputs .= "$vlog\n";
    }
}

$ver_inputs .= '-f ${HDK_COMMON_DIR}/verif/tb/filelists/tb.${SIMULATOR}.f

${TEST_NAME}
';
    print TOP_VIVADO_F $ver_inputs; 
    close TOP_VIVADO_F;

    # Generate VHDL source file
    if (scalar(@vhdls) > 0){  
        copy("$cl_dir/verif/scripts/top.vivado.vhd.f","$cl_dir/verif/scripts/top.vivado.vhd.f.bk") or die "Copy failed: $!";

        open TOP_VIVADO_F, ">$cl_dir/verif/scripts/top.vivado.vhd.f";
        my $vhd_inputs = "";
        foreach my $vhdl(@vhdls) {
            $vhd_inputs .= "$vhdl\n";
        }

        print TOP_VIVADO_F $vhd_inputs;
        close TOP_VIVADO_F;

    }

    # Generate vivado Makefile
    copy("$cl_dir/verif/scripts/Makefile.vivado","$cl_dir/verif/scripts/Makefile.vivado.bk") or die "Copy failed: $!";
    open MAKEFILE, ">$cl_dir/verif/scripts/Makefile.vivado";

    my $inc_vhdl = '';
    if (scalar(@vhdls) > 0) {
        $inc_vhdl = "\t".'cd $(SIM_DIR) && xvhdl  --initfile $(XILINX_VIVADO)/data/xsim/ip/xsim_ip.ini --work xil_defaultlib --relax -f $(SCRIPTS_DIR)/top.vivado.vhd.f
    ';
    }

    my $makefile='compile:
	mkdir -p $(SIM_DIR)
	cd $(SIM_DIR) && xsc $(C_FILES) --additional_option "-I$(C_SDK_USR_INC_DIR)" --additional_option "-I$(C_SDK_USR_UTILS_DIR)" --additional_option "-I$(C_COMMON_DIR)/include" --additional_option "-I$(C_COMMON_DIR)/src" --additional_option "-I$(C_INC_DIR)"  --additional_option "-DVIVADO_SIM" --additional_option "-DSV_TEST" --additional_option "-DDMA_TEST"
	cd $(SIM_DIR) && xvlog --sv -m64 --define DMA_TEST --initfile $(XILINX_VIVADO)/data/xsim/ip/xsim_ip.ini --work xil_defaultlib --relax -f $(SCRIPTS_DIR)/top.vivado.f'.$inc_vhdl.'
	cd $(SIM_DIR) && xelab -m64 --initfile $(XILINX_VIVADO)/data/xsim/ip/xsim_ip.ini --timescale 1ps/1ps --debug typical --relax --mt 8 -L axi_clock_converter_v2_1_14 -L axi_clock_converter_v2_1_11 -L generic_baseblocks_v2_1_0 -L axi_infrastructure_v1_1_0 -L axi_register_slice_v2_1_15 -L axi_register_slice_v2_1_12 -L fifo_generator_v13_2_1 -L fifo_generator_v13_1_4 -L axi_data_fifo_v2_1_11 -L axi_crossbar_v2_1_13 -L axi_dwidth_converter_v2_1_12 -L blk_mem_gen_v8_3_6 -L xil_defaultlib -L unisims_ver -L unimacro_ver -L secureip -L xpm -sv_lib dpi --snapshot tb xil_defaultlib.tb xil_defaultlib.glbl xil_defaultlib.$(TEST)

compile_chk:
	mkdir -p $(SIM_DIR)
	cd $(SIM_DIR) && xsc $(C_FILES) --additional_option "-I$(C_SDK_USR_INC_DIR)" --additional_option "-I$(C_SDK_USR_UTILS_DIR)" --additional_option "-I$(C_COMMON_DIR)" --additional_option "-I$(C_INC_DIR)" --additional_option "-DVIVADO_SIM" --additional_option "-DSV_TEST"
	cd $(SIM_DIR) && xvlog --sv -m64 -d ENABLE_PROTOCOL_CHK --initfile $(XILINX_VIVADO)/data/xsim/ip/xsim_ip.ini --work xil_defaultlib --relax -f $(SCRIPTS_DIR)/top.vivado.f'.$inc_vhdl.'
	cd $(SIM_DIR) && xelab -m64 -d ENABLE_PROTOCOL_CHK --initfile $(XILINX_VIVADO)/data/xsim/ip/xsim_ip.ini --timescale 1ps/1ps --debug typical --relax --mt 8 -L axi_protocol_checker_v1_1_12 -L axi_clock_converter_v2_1_11 -L generic_baseblocks_v2_1_0 -L axi_infrastructure_v1_1_0 -L axi_register_slice_v2_1_12 -L fifo_generator_v13_1_4 -L axi_data_fifo_v2_1_11 -L axi_crossbar_v2_1_13 -L axi_dwidth_converter_v2_1_12 -L blk_mem_gen_v8_3_6 -L xil_defaultlib -L unisims_ver -L unimacro_ver -L secureip -L xpm -sv_lib dpi --snapshot tb xil_defaultlib.tb xil_defaultlib.glbl xil_defaultlib.$(TEST)

run:

ifeq ($(TEST),test_null)
	cd $(SIM_DIR) && xsim -R -log $(C_TEST).log -tclbatch $(SCRIPTS_DIR)/waves.tcl tb
else
	cd $(SIM_DIR) && xsim -R -log $(TEST).log -tclbatch $(SCRIPTS_DIR)/waves.tcl tb
endif
';
    print MAKEFILE $makefile;
    close MAKEFILE;
}
1;
