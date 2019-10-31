#!/usr/bin/perl
use strict;
use warnings;
use Cwd;
use File::Copy;

my $dir = getcwd;
my $json_fn = $ARGV[0];
my $soc_name = $json_fn;
$soc_name =~ s/.json//;
my $rdir = $ENV{'RDIR'};

my $postfix="";

my $num_args = $#ARGV + 1;
if ($num_args > 1) {
  $postfix= $ARGV[1];
}

if ((not defined($rdir)) or $rdir eq '') {
    print("Please source sourceme-f1-manager.sh!\n");
    exit();
}

if (not defined($json_fn)){
    print("Please specify a json config file\!\n");
    exit();
}

my $scripts_dir = $rdir.'/tools/centrifuge/scripts/';
require $scripts_dir.'parse_json.pl';
require $scripts_dir.'generate_accel.pl';
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
generate_accel(\@Accel_tuples);
# Generate build.sbt under firesim/sim
generate_build_sbt($soc_name, \%hls_bm);
# Generate HLSConfig file for RoCC Accelerators
generate_config(\@RoCC_names, \@TLL2_names, $postfix);

# F1 
#generate_f1_scripts(\%hls_bm);
#generate_xsim_scripts(\%hls_bm);
#compile_xsim_libs($postfix, "clean", 0);
#compile_replace_rtl($postfix, "clean", 0);
#print_xsim_cmd($postfix, 0);

# Ax machines
#compile_vcs("clean");
#copy_verilog(\%hls_bm, "$rdir/sim/generated-src/f1/FireSimHLS-HLSFireSimRocketChipConfig-FireSimConfig/FPGATop.v");

sub print_xsim_cmd{
    my $postfix= $_[0];
    my $with_nic = $_[1];
    my $nic = "NoNIC";
    if ($with_nic) {
        $nic = "";
    } 
    print("\n");
    print("Source Full Env:\n source sourceme-f1-full.sh\n");
    print("XSim Compile:\n".'cd $RDIR/sim '."&& make DESIGN=FireSimHLS$nic TARGET_CONFIG=HLSFireSimRocketChipConfig$postfix PLATFORM_CONFIG=FireSimConfig xsim\n"); 
    #cl_FireSimHLSNoNIC-HLSFireSimRocketChipConfig-FireSimConfig/verif
    print("Remove Sim Folder:\n".'rm -rf cl_'."FireSimHLS$nic-HLSFireSimRocketChipConfig$postfix-FireSimConfig/verif/sim\n"); 
    print("XSim Run Driver:\n".'cd $RDIR/sim '."&& make DESIGN=FireSimHLS$nic TARGET_CONFIG=HLSFireSimRocketChipConfig$postfix PLATFORM_CONFIG=FireSimConfig xsim-dut\n"); 
    print("XSim Run Test:\n".'cd $RDIR/sim '."&& make DESIGN=FireSimHLS$nic TARGET_CONFIG=HLSFireSimRocketChipConfig$postfix PLATFORM_CONFIG=FireSimConfig run-xsim SIM_BINARY=".'$RDIR/sim/target-rtl/firechip/hls_${PGM}_${FUNC}/src/main/c/${PGM}.riscv'); 
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
    chdir("$rdir/sim");
    system("make DESIGN=FireSimHLS$nic TARGET_CONFIG=HLSFireSimRocketChipConfig$postfix PLATFORM_CONFIG=FireSimConfig $clean xsim");
}

sub compile_replace_rtl{
    my $postfix= $_[0];
    my $clean = $_[1];
    my $with_nic = $_[2];
    my $nic = "NoNIC";
    if ($with_nic) {
        $nic = "";
    } 
    chdir("$rdir/sim");
    system("make DESIGN=FireSimHLS$nic TARGET_CONFIG=HLSFireSimRocketChipConfig$postfix PLATFORM_CONFIG=FireSimConfig $clean replace-rtl");
}

sub compile_vcs{
    my $clean = $_[0];
    chdir("$rdir/sims/vcs");
    system("make $clean debug CONFIG=");
}

sub copy_verilog{
    my %bm_path = %{$_[0]};
    my $FPGATop_path = $_[1];

    while(my($bm, $path) = each %bm_path) {
      system("cat $path/src/main/verilog/*.v >> $FPGATop_path");
    }
}


