#!/usr/bin/perl
use strict;
use warnings;
use Cwd;
use File::Copy;
use File::Path 'rmtree';

my $dir = getcwd;

################# Parse Arguments ####################
my $json_fn = $ARGV[0];
my $rdir = $ENV{'RDIR'};

my $task = $ARGV[1];

my $postfix="";
my $num_args = $#ARGV + 1;
if ($num_args > 2) {
  $postfix= $ARGV[2];
}

if ((not defined($rdir)) or $rdir eq '') {
    print("Please source centrifuge/env.sh!\n");
    exit();
}

if (not defined($json_fn)){
    print("Please specify a json config file\!\n");
    exit();
}
my $soc_name = $json_fn;
$soc_name =~ s/.json//;

my $arg_options = "Options: 
        all -- Generate all HW/SW interfaces,configs, and scripts for the accelerator SoC
        accel -- Rerun accelerator HW/SW interface generation  
        accel_hls -- Rerun HLS generation
        accel_chisel -- Rerun chisel wrapper generation (triggers accel_hls to run)
        accel_sw -- Rerun C wrapper generation (triggers accel_hls to run)
        build_sbt -- Regenerate build.sbt
        config -- Regenerate HLSConfig.scala file in examples and firechip 
        compile_sw_bm -- Compile bare-metal accelerator invocation code
        f1_scripts -- Regenerate scripts for including HLS generated Verilog in FireSim
        xsim_scripts -- Regenerate scripts for including HLS generated Verilog in FireSim XSim
        run_vcs -- Regenerate vcs imulation
        run_verilator -- Regenerate verilator imulation
        clean -- Delete accelerator directory
        ";

my @valid_task_list = qw(all accel accel_hls accel_chisel accel_sw build_sbt config f1_scripts xsim_scripts run_vcs run_verilator clean compile_sw_bm);
if (not defined($task) or not grep( /^$task$/, @valid_task_list)){
    print("Please specify a task\!\n");
    print($arg_options);
    exit();
}

if ($task eq 'clean') {
    rmtree(["$rdir/generators/$soc_name/"]);
    exit();
}

my @task_list = (); 
my @accel_task_list = qw(accel accel_hls accel_chisel accel_sw build_sbt config);
if ($task eq 'all') {
    push (@task_list, @accel_task_list);
    push (@task_list, qw(build_sbt config f1_scripts xsim_scripts));
} elsif ($task eq 'accel') {
    push (@task_list, @accel_task_list);
} elsif ($task eq 'accel_hls') {
    push (@task_list, qw(accel accel_hls));
} elsif ($task eq 'accel_chisel') {
    push (@task_list, qw(accel accel_hls accel_chisel));
} elsif ($task eq 'accel_sw') {
    push (@task_list, qw(accel accel_hls accel_sw));
} elsif ($task eq 'build_sbt') {
    push (@task_list, qw(build_sbt));
} elsif ($task eq 'config') {
    push (@task_list, qw(config));
} elsif ($task eq 'compile_sw_bm') {
    push (@task_list, qw(compile_sw_bm));
} elsif ($task eq 'f1_scripts') {
    push (@task_list, qw(f1_scripts));
} elsif ($task eq 'xsim_scripts') {
    push (@task_list, qw(xsim_scripts));
} elsif ($task eq 'run_vcs') {
    push (@task_list, qw(run_vcs));
} elsif ($task eq 'run_verilator') {
    push (@task_list, qw(run_verilator));
}


my %tasks = ();
foreach my $task (@valid_task_list) {
    $tasks{$task} = 0;
}

foreach my $task (@task_list) {
    $tasks{$task} = 1  
}
################# Run ####################
my $DESIGN='FireSimTopWithHLS';
my $TARGET_CONFIG='HLSFireSimRocketChipConfig';
my $PLATFORM_CONFIG='BaseF1Config_F90MHz';

my $CONFIG='HLSRocketConfig';
my $TOP='TopWithHLS';

my $scripts_dir = $rdir.'/tools/centrifuge/scripts/';
require $scripts_dir.'parse_json.pl';
require $scripts_dir.'generate_accel.pl';
require $scripts_dir.'compile_sw_bm.pl';
require $scripts_dir.'generate_build_sbt.pl';
require $scripts_dir.'generate_config.pl';
require $scripts_dir.'generate_f1_scripts.pl';
require $scripts_dir.'generate_xsim_scripts.pl';

# Parse Json file
my ($RoCC_ref, $TLL2_ref) = parse_json($json_fn);
my @RoCC_accels = @$RoCC_ref;
my @TLL2_accels = @$TLL2_ref;

my %hls_bm = ();
my @Accel_tuples = ();
my @RoCC_names = ();
my @TLL2_names = ();
my $idx = 0;
foreach my $RoCC_accel (@RoCC_accels){
  my @arr = @{$RoCC_accel};
  my $pgm = $arr[0];
  my $func = $arr[1];

  my $bm_path = "";
  if (scalar @arr > 2) {
    $bm_path = $arr[2];  
  } else {
    $bm_path = $rdir."/generators/$soc_name/hls_$pgm"."_$func"; ; 
  }
  my $prefix = "rocc".$idx."_";
  # 3rd arg is_rocc is set to 1
  push(@Accel_tuples, [$pgm, $func, $bm_path, 1, $idx, $prefix]);
  $func=$prefix.$func;
  $hls_bm{"hls_$func"} = $bm_path; 
  push(@RoCC_names, $func);
  $idx += 1;
}

$idx = 0;
foreach my $TLL2_accel (@TLL2_accels){
  my @arr = @{$TLL2_accel};
  my $pgm = $arr[0];
  my $func = $arr[1];
  my $addr = $arr[2];

  my $bm_path = "";
  if (scalar @arr > 3) {
    $bm_path = $arr[3];  
  } else {
    $bm_path = $rdir."/generators/$soc_name/hls_$pgm"."_$func"; ; 
  }

  my $prefix = "tl".$idx."_";
  push(@Accel_tuples, [$pgm, $func, $bm_path, 0, $addr, $prefix]);
  $func=$prefix.$func;
  $hls_bm{"hls_$func"} = $bm_path; 
  push(@TLL2_names, $func);
  $idx += 1;
}

# Generate the verilog and chisel code
if ($tasks{'accel'}){
    generate_accel(\@Accel_tuples, \%tasks);
    # SW
    # Copy Makefile Templates
    system("cp $scripts_dir/sw_aux/makefiles/* $rdir/generators/$soc_name/");
}

if ($tasks{'compile_sw_bm'}){
    compile_sw_bm(\@Accel_tuples, \%tasks);
}

# Generate build.sbt under firesim/sim
if ($tasks{'build_sbt'}){
    generate_build_sbt($soc_name, \%hls_bm);
}
# Generate HLSConfig file for RoCC Accelerators
if ($tasks{'config'}){
    generate_config(\@RoCC_names, \@TLL2_names, $postfix);
}

# F1 
if ($tasks{'f1_scripts'}){
    generate_f1_scripts(\%hls_bm);
}
if ($tasks{'xsim_scripts'}){
    generate_xsim_scripts(\%hls_bm);
}
if ($tasks{'f1_scripts'} or $tasks{'xsim_scripts'}){
    compile_replace_rtl($postfix, "clean", 0);
}
if ($tasks{'xsim_scripts'}){
    compile_xsim_libs($postfix, "clean", 0);
    print_xsim_cmd($postfix, 0);
}

# Ax machines
if ($tasks{'run_vcs'}){
    compile_vcs("clean");
    copy_verilog(\%hls_bm, "$rdir/sims/vcs/generated-src/example.TestHarness.$CONFIG/example.TestHarness.$CONFIG.top.v");
    compile_vcs("");
}

if ($tasks{'run_verilator'}){
    compile_verilator("clean");
    copy_verilog(\%hls_bm, "$rdir/sims/verilator/generated-src/example.TestHarness.$CONFIG/example.TestHarness.$CONFIG.top.v");
    compile_verilator("");
}

sub print_xsim_cmd{
    my $postfix= $_[0];
    my $with_nic = $_[1];
    my $nic = "NoNIC";
    if ($with_nic) {
        $nic = "";
    } 
    print("\n");
    print("Source Full Env:\n source sourceme-f1-full.sh\n");
    print("XSim Compile:\n".'cd $RDIR/sims/firesim/sim '."&& make DESIGN=$DESIGN$nic TARGET_CONFIG=$TARGET_CONFIG$postfix PLATFORM_CONFIG=$PLATFORM_CONFIG xsim\n"); 
    #cl_FireSimHLSNoNIC-HLSFireSimRocketChipConfig-FireSimConfig/verif
    print("Remove Sim Folder:\n".'rm -rf cl_'."$DESIGN$nic-$TARGET_CONFIG$postfix-$PLATFORM_CONFIG/verif/sim\n"); 
    print("XSim Run Driver:\n".'cd $RDIR/sims/firesim/sim '."&& make DESIGN=$DESIGN$nic TARGET_CONFIG=$TARGET_CONFIG$postfix PLATFORM_CONFIG=$PLATFORM_CONFIG xsim-dut\n"); 
    print("XSim Run Test:\n".'cd $RDIR/sims/firesim/sim '."&& make DESIGN=$DESIGN$nic TARGET_CONFIG=$TARGET_CONFIG$postfix PLATFORM_CONFIG=$PLATFORM_CONFIG run-xsim SIM_BINARY=".'$RDIR/sims/firesim/sim/target-rtl/firechip/hls_${PGM}_${FUNC}/src/main/c/${PGM}.riscv'); 
    print("\n");
    #print("LD_LIBRARY_PATH=output/f1/FireSimHLS$nic-HLSFireSimRocketChipConfig$postfix-FireSimConfig/ output/f1/FireSimHLS$nic-HLSFireSimRocketChipConfig$postfix-FireSimConfig/FireSimHLS$nic-f1 ".'+mm_readLatency=10 +mm_writeLatency=10 +mm_readMaxReqs=4 +mm_writeMaxReqs=4  +netburst=8 +slotid=0 $RDIR/sim/target-rtl/firechip/hls_${PGM}_${FUNC}/src/main/c/${PGM}.riscv');
}

sub compile_xsim_libs{
    my $postfix= $_[0];
    my $clean = $_[1];
    my $with_nic = $_[2];
    my $nic = "NoNIC";
    if ($with_nic) {
        $nic = "";
    } 
    chdir("$rdir/sims/firesim/sim");
    system("make DESIGN=$DESIGN$nic TARGET_CONFIG=$TARGET_CONFIG$postfix PLATFORM_CONFIG=$PLATFORM_CONFIG $clean xsim");
}

sub compile_replace_rtl{
    my $postfix= $_[0];
    my $clean = $_[1];
    my $with_nic = $_[2];
    my $nic = "NoNIC";
    if ($with_nic) {
        $nic = "";
    } 
    chdir("$rdir/sims/firesim/sim");
    system("make DESIGN=$DESIGN$nic TARGET_CONFIG=$TARGET_CONFIG$postfix PLATFORM_CONFIG=$PLATFORM_CONFIG $clean replace-rtl");
}

sub compile_verilator{
    my $clean = $_[0];
    chdir("$rdir/sims/verilator");
    system("make $clean CONFIG=$CONFIG TOP=$TOP debug -j16");
}

sub compile_vcs{
    my $clean = $_[0];
    chdir("$rdir/sims/vcs");
    system("make $clean CONFIG=$CONFIG TOP=$TOP debug -j16");
}

sub copy_verilog{
    my %bm_path = %{$_[0]};
    my $FPGATop_path = $_[1];

    while(my($bm, $path) = each %bm_path) {
      system("cat $path/src/main/verilog/*.v >> $FPGATop_path");
    }
}

