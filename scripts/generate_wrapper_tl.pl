#!/usr/bin/perl
use warnings;
use strict;
use Cwd;
use File::Copy;
use List::Util qw(first);
use Tie::IxHash;

# Inputs: file_name, func_name, func_base_addr, prefix(Optional)
my $dir = getcwd;
my $file_name = $ARGV[0];
my $func_name = $ARGV[1];
my $func_base_addr = $ARGV[2];

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

my $wrapper_func_name = $func_name."_wrapper";
my $wrapper_header= "bm_wrapper.h";

if ($prefix) {
  $func_name  = $prefix.$func_name;
}

my $bm_inc_path = $rdir."/tools/centrifuge/scripts/sw_aux/sw_helper/";
#############################PARSE Verilog##############################

my %var_dict;
tie %var_dict, "Tie::IxHash";
my $verilog_file = "$dir/../verilog/$func_name"."_control_s_axi.v";
print "Parsing ".$verilog_file."\n";
# parse the verilog file to get the info we need
if(!open VERILOG, "$verilog_file"){
	print $!;
} else {
  my $start = 0;
  my $line = undef;
	while(<VERILOG>){
		$line = $_;

       if($line =~ m/------------------------Parameter----------------------/){
          $start = 0;
       }
        if($start){

          if($line =~ m/(0x\S+) : Data signal of (\S+)/){
            my $base_addr = $1;
            my $var = $2;  
            #print("$base_addr : $var\n");
            if (exists $var_dict{$var}) {
              push (@{$var_dict{$var}}, $base_addr); 
            } else {
              my @addr = ();
              push (@addr, $base_addr);
              $var_dict{$var} = \@addr;
            }
          }


        } 
       if($line =~ m/------------------------Address Info------------------/){
          $start= 1;
        }
  }
}


#############################GENERATE Software Bare-metal Wrappers##############################
# We want ordered hash so we didn't add this piece of code into a func
#sub generate_bm_wrapper {
#    my %var_dict=%{$_[0]};
#    tie %var_dict, "Tie::IxHash";
#    my $func_base_addr = $_[1];
    foreach my $var (keys %var_dict) { 
      print($var.": ");

      my @addr = @{$var_dict{$var}};
      foreach my $base_addr(@addr) {

        print($base_addr."\t");
      }
      print("\n");
    }
    my $wrapper = '#include "'.$bm_inc_path.'mmio.h"'."\n";

    $wrapper .= '#define ACCEL_BASE '.$func_base_addr."\n";

    $wrapper .= "#define AP_DONE_MASK 0b10\n";
    $wrapper .= "#define ACCEL_INT 0x4\n";
    foreach my $var (keys %var_dict) { 

      my @addr = @{$var_dict{$var}};
      my $idx = 0;

      foreach my $base_addr(@addr) {
        $wrapper .="#define "."ACCEL_$var"."_$idx"." $base_addr\n";
        $idx +=1;
      }
    }
     
    my $ap_return = 0;
    my $ap_return_type = "uint32_t";
    if (exists $var_dict{"ap_return"}) {
      my $size=@{$var_dict{"ap_return"}};
      if ($size == 2){
        $ap_return_type = "uint64_t";
      }
      $ap_return = 1;
    } 
    
    if ($ap_return){
      $wrapper .= $ap_return_type." $wrapper_func_name(";
    } else {
      $wrapper .="void $wrapper_func_name(";
    }

    my @arglist=();
    foreach my $var (keys %var_dict) { 
      if ($var eq "ap_return") {
        next;
      }

      my $var_type = "uint32_t";
      my $size=@{$var_dict{$var}};
      if ($size == 2){
        $var_type = "uint64_t";
      } 
      push(@arglist, "$var_type $var");
    }
    
    my $args = join ', ', @arglist;
    $wrapper.= $args.") {";

    $wrapper.= '
    // Disable Interrupt
    reg_write32(ACCEL_BASE + ACCEL_INT, 0x0);
';

    foreach my $var (keys %var_dict) { 
      if ($var eq "ap_return") {
        next;
      }

      my @addr = @{$var_dict{$var}};
      my $idx = 0;
      foreach my $base_addr(@addr) {
        my $shift = "";
        if ($idx == 1){
          $shift = " >> 32";
        }elsif($idx > 1){
          die "Index exceeds limit!\n";
        }
        $wrapper .="    reg_write32(ACCEL_BASE + ACCEL_$var"."_$idx, (uint32_t) ($var$shift));\n";
        $idx +=1;
      }
    }
    

    $wrapper .='
    // Write to ap_start to start the execution 
    reg_write32(ACCEL_BASE, 0x1);

    // Done?
    int done = 0;
    while (!done){
        done = reg_read32(ACCEL_BASE) & AP_DONE_MASK;
    }
';

    # If there a return value
    if ($ap_return){
      my @addr = @{$var_dict{"ap_return"}};

      $wrapper .= "
    $ap_return_type ret_val = 0;\n";
      my $idx = 0;
      foreach my $base_addr(@addr) {
        my $shift = "";
        if ($idx == 1){
          $shift = " >> 32";
        }elsif($idx > 1){
          die "Index exceeds limit!\n";
        }
        $wrapper .="    ret_val = (reg_read32(ACCEL_BASE + ACCEL_ap_return"."_$idx)$shift) | ret_val;\n";
        $idx +=1;
      }
      $wrapper .= "    return ret_val;\n";
    }

    $wrapper .="}\n";
    open FILE, "> $wrapper_header";
    print FILE $wrapper;
#}

#generate_bm_wrapper(\%var_dict, $func_base_addr);
