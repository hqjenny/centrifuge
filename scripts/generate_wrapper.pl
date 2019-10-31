#!/usr/bin/perl
use warnings;
use strict;
use Cwd;
use File::Copy;
use List::Util qw(first);

# Inputs: file_name, func_name, rocc_index, prefix(Optional)
my $dir = getcwd;
my $file_name = $ARGV[0];
my $func_name = $ARGV[1];
my $rocc_index= $ARGV[2];

my $prefix = undef;

my $num_args = $#ARGV + 1;
if ($num_args > 3) {
  $prefix = $ARGV[3];
}

my $rdir = $ENV{'RDIR'};
#print $rdir;
if ((not defined($rdir)) or $rdir eq '') {
    print("Please source sourceme-f1.sh!\n");
    exit();
}
my $bm_path = $rdir."/sim/target-rtl/firechip/hls_$file_name"."_$func_name";
my $wrapper_func_name = $func_name."_wrapper";
my $wrapper_header= "bm_wrapper.h";
if ($prefix) {
  $func_name  = $prefix.$func_name;
}

#############################PARSE Verilog##############################
my $verilog_file = "$dir/../verilog/$func_name".".v";
my $line = undef;
my @verilog_input = ();
my @verilog_input_size = ();
my @verilog_output = ();
my @verilog_output_size = ();

print "Parsing ".$verilog_file."\n";
# parse the verilog file to get the info we need
if(!open VERILOG, "$verilog_file"){
	print $!;
} else {
	while(<VERILOG>){
		$line = $_;
		if($line =~ m/^\s*input\s+(.*)/){
			my $input = $1;
			#print "input:$input\n";
			if($input =~ m/\s*\[(.*):(.*)\]\s*(.*)\s*;/){	
				my $end = $1;
				my $start = $2;
				my $input_name = $3;
				#print "here!"."$input_name\n";
				push (@verilog_input, $input_name);
				my $size = $end - $start + 1;
				push(@verilog_input_size, $size);
			}elsif ($input =~ m/\s*(.*)\s*;/){
				my $input_name = $1;
				#print "here!"."$input_name\n";
				push (@verilog_input, $input_name);
				push(@verilog_input_size, 1);
			}

		}elsif($line =~ m/^\s*output\s+(.*)/){
			my $output = $1;
			#print "output:$output\n";
			if($output =~ m/\s*\[(.*):(.*)\]\s*(.*)\s*;/){	
				my $end = $1;
				my $start = $2;
				my $output_name = $3;
				#print "here!"."$output_name\n";
				push(@verilog_output, $output_name);
				my $size = $end - $start + 1;
				push(@verilog_output_size, $size);
			}elsif ($output =~ m/\s*(.*)\s*;/){
				my $output_name = $1;
				#print "here!"."$output_name\n";
				push (@verilog_output, $output_name);
				push(@verilog_output_size, 1);
			}
		}
	}
    print("Inputs:");
    my $in_str = join ' ', @verilog_input;
    print $in_str."\n";
    print("Outputs:");
    my $out_str = join ' ', @verilog_output;
    print $out_str."\n";
}

#creat scala folder
my $scala_dir = "$dir/../scala";
mkdir $scala_dir unless (-d $scala_dir);

##############################################################################################################################
print "Generating BlackBox file ...\n";
# should be under scala folder

my $blackbox1 = "
package hls_test_c
import Chisel._
import freechips.rocketchip.config.{Parameters, Field}
import freechips.rocketchip.tile._
import freechips.rocketchip.util._
import vivadoHLS._

class test_c() extends BlackBox() {
";
$blackbox1 =~ s/test_c/$func_name/g;


my $i = undef;
my $bb_body = "";

# now if the input name does not start with ap, we assume it is an arg
my $ap_return = 0;
my $ap_clk = 0;
my $ap_rst = 0;
my @verilog_input_scalar = ();
my %verilog_input_pointer = ();
my @verilog_input_pointer_arg = ();  # An ordered list of args 

my $arg_count = 0;
my @sindices = ();
my @pindices = ();

for( $i = 0; $i < @verilog_input; $i = $i + 1 ){
	my $input_name = $verilog_input[$i]; 
	my $input_size = $verilog_input_size[$i];

	if ($input_name =~ m/ap_clk(.*)/){
		$ap_clk = 1;
	}


	elsif ($input_name =~ m/ap_rst(.*)/){
		$ap_rst = 1;
	}

    # If the input is a ap_bus port, the signals should match the following format 
    # There should be 3 different input signals 
    elsif($input_name =~ m/(\S+)_req_full_n/ or $input_name =~ m/(\S+)_rsp_empty_n/ or $input_name =~ m/(\S+)_datain/){
        my $arg_name = $1;
        if ($input_name =~ m/(\S+)_datain/) {
          push(@pindices, $arg_count);
          $arg_count = $arg_count + 1;
          push(@verilog_input_pointer_arg, $arg_name);
        }
        if (defined $verilog_input_pointer{$arg_name}) {
            $verilog_input_pointer{$arg_name} += 1;
        } else {
            $verilog_input_pointer{$arg_name} = 1;
        }
    }
	elsif(!($input_name =~ m/ap_(,*)/)){
		push (@verilog_input_scalar, $input_name);
    push(@sindices, $arg_count);
    $arg_count = $arg_count + 1;
	} 
    else{
        print("Not func args: $input_name\n");
    }
}

#foreach my $arg (keys %verilog_input_pointer) {
foreach my $arg (@verilog_input_pointer_arg) {
    print("pointer_arg: $arg\n");  
}
my $hash_count = keys %verilog_input_pointer;
print("hash_count: $hash_count\n");
if(@verilog_input_scalar + $hash_count> 2){
    print "verilog_input_scalar: ";
    my $in_str = join ' ', @verilog_input_scalar;
    print $in_str."\n";
	die "Only accept function with no more than 2 arguments!\n";
}

foreach my $arg (keys %verilog_input_pointer) {
    if ($verilog_input_pointer{$arg} ne 3) {
        die "The AP bus interfance did not generate expected number of inputs!\n";
    }
}

for( $i = 0; $i < @verilog_output; $i = $i + 1 ){

	my $output_name = $verilog_output[$i]; 
	my $output_size = $verilog_output_size[$i];

	if ($output_name =~ m/ap_return(.*)/){
		$ap_return = 1;
	}

	$bb_body = $bb_body."\tio.".$output_name.".setName(\"".$output_name."\")\n";
}

if ($ap_clk eq 1){
	$bb_body = $bb_body."addClock(Driver\.implicitClock)\n".'renameClock("clk", "ap_clk")'."\n";
}

if ($ap_rst eq 1){
	$bb_body = $bb_body.'renameReset("ap_rst")'."\n";
}

my $bb_def = "class HLS$func_name"."Blackbox() extends Module {\n";

# Scalar IO Parameter
my @sdata_widths = (); 
#my @sindices = ();
#my $sidx = 0;
foreach my $arg (@verilog_input_scalar) {
    my $sdata_idx = first { $verilog_input[$_] eq $arg} 0..$#verilog_input;  
    my $sdata_width = $verilog_input_size[$sdata_idx];
    push(@sdata_widths, $sdata_width);
    #push(@sindices, $sidx);
    #$sidx += 1;
}
my $sindices_str = join ',',@sindices;
my $sdata_widths_str = join ',',@sdata_widths;
print "scalar data_widths: $sdata_widths_str\n";

$bb_def .= "\tval scalar_io_dataWidths = List($sdata_widths_str)\n";
$bb_def .= "\tval scalar_io_argLoc = List($sindices_str) //Lists the argument number of the scalar_io\n";

# Pointer IO Parameter
my @addr_widths = (); 
my @data_widths = (); 
#my @indices = ();
my $idx = 0;
foreach my $arg (sort keys %verilog_input_pointer) {
    my $addr_signal = $arg."_address";
    my $data_signal = $arg."_dataout";
    my $addr_idx = first { $verilog_output[$_] eq $addr_signal } 0..$#verilog_output;  
    my $data_idx = first { $verilog_output[$_] eq $data_signal } 0..$#verilog_output;  
    #my $addr_width =  $verilog_output_size[$addr_idx]; 
    my $addr_width = "64";

    my $data_width =  $verilog_output_size[$data_idx]; 
    push(@addr_widths, $addr_width);  
    push(@data_widths, $data_width);  
    #push(@indices, $idx);
    $idx += 1;
}
#my $indices_str = join ',',@indices;
my $pindices_str = join ',',@pindices;
my $addr_widths_str = join ',',@addr_widths;
print "addr_widths: $addr_widths_str\n";
my $data_widths_str = join ',',@data_widths;
print "data_widths: $data_widths_str\n";


foreach my $arg (@verilog_input_pointer_arg) {
    print("pointer_arg: $arg\n");  
}

my $wrapper ='
#ifdef CUSTOM_INST 
#include "rocc.h"
#endif
';

my $return_type = "void ";
if($ap_return){
  $return_type = "uint64_t "; 
}

my $total_args = @verilog_input_scalar + $hash_count;
$wrapper .= "$return_type $wrapper_func_name(";

my @args = ();
foreach my $arg (@verilog_input_scalar) {
  push(@args, $arg);
}
foreach my $arg (@verilog_input_pointer_arg) {
  push(@args, $arg);
}

my $arg_str = join ', ', @args;
my $i = 0;
foreach my $arg (@args) {
  if ($i != 0){
    $wrapper .=", "
  } 
  $wrapper .="uint64_t $arg";

  $i=1;
}
$wrapper .= ") {
";

if($ap_return){
  $wrapper .= "    uint64_t ret_val;\n";
}
$wrapper .="
  #ifdef CUSTOM_INST
    #define XCUSTOM_ACC ";
$wrapper .= $rocc_index."\n";

if ($ap_return){ 
  if ($total_args == 0) {
    $wrapper.="        ROCC_INSTRUCTION_D(XCUSTOM_ACC, ret_val, 0);\n";
  } elsif ($total_args == 1) {
    $wrapper.="        ROCC_INSTRUCTION_DS(XCUSTOM_ACC, ret_val, $arg_str, 0);\n";
  } else {
    $wrapper.="        ROCC_INSTRUCTION_DSS(XCUSTOM_ACC, ret_val, $arg_str, 0);\n";
  }
} else{
  if ($total_args == 0) {
    $wrapper.="        ROCC_INSTRUCTION(XCUSTOM_ACC, 0);\n";
  } elsif ($total_args == 1) {
    $wrapper.="        ROCC_INSTRUCTION_S(XCUSTOM_ACC, $arg_str, 0);\n";
  } else {
    $wrapper.="        ROCC_INSTRUCTION_SS(XCUSTOM_ACC, $arg_str, 0);\n";
  }
}
$wrapper .= "      ROCC_BARRIER();\n";
$wrapper.="  #endif\n";
if($ap_return){
  $wrapper .= "    return ret_val;\n";
}
$wrapper.="}";

open FILE, "> $wrapper_header";
print FILE $wrapper;

