#!/usr/bin/perl
use warnings;
use strict;
use Cwd;
use File::Copy;
use List::Util qw(first);

# Inputs: file_name, func_name, func_base_addr, prefix(Optional)
my $dir = getcwd;
my $file_name = $ARGV[0];
my $func_name = $ARGV[1];
my $func_base_addr  = $ARGV[2];
my $rdir = $ENV{'RDIR'};

my $prefix = undef;
my $i = undef;

my $num_args = $#ARGV + 1;
if ($num_args > 3) {
  $prefix = $ARGV[3];
}

#my $bm_path = $rdir."/sim/target-rtl/firechip/hls_$file_name"."_$func_name";
if ($prefix) {
  $func_name  = $prefix.$func_name;
}

#print $rdir;
if ((not defined($rdir)) or $rdir eq '') {
    print("Please source sourceme-f1.sh!\n");
    exit();
}

# my $build_sbt = '
# organization := "edu.berkeley.cs"
# 
# version := "1.0"
# 
# name := "hls_test_c"';
# 
# $build_sbt=~ s/test_c/$func_name/g;
# my $build_sbt_path= "$bm_path/"."build.sbt";
# open BUILD, ">$build_sbt_path";
# print BUILD $build_sbt;
# close BUILD;

my $verilog_file = "$dir/../verilog/$func_name".".v";
my $line = undef;
my @verilog_param = ();
my @param_val = ();
my @verilog_input = ();
my @verilog_input_size = ();
my @verilog_output = ();
my @verilog_output_size = ();

#my $m_axi_data_width = undef;
#my $s_axi_data_width = undef;

my @bus_names=(); 
my @m_axi_data_widths = ();
my $s_axi_data_width = undef;

print "Parsing ".$verilog_file."\n";
# parse the verilog file to get the info we need
if(!open VERILOG, "$verilog_file"){
	print $!;
} else {
	while(<VERILOG>){
		$line = $_;

        # Match AXI4 parameter 
        if($line =~ m/parameter\s+(C_\S+) =\s+(.*);/){
            my $param = $1;
            my $val = $2;
            $param .="";
            if($param =~ m/C_M_AXI_(\S+)_DATA_WIDTH/){
                my $bus_name = lc $1;
                #$m_axi_data_width = $val; 
                push(@bus_names, $bus_name);
                push(@m_axi_data_widths, $val);
            }
            if ($param eq "C_S_AXI_DATA_WIDTH") {
                $s_axi_data_width = $val; 
            }
            push (@verilog_param, $param); 
            push (@param_val, $val); 
        } elsif($line =~ m/^\s*input\s+(.*)/){
			my $input = $1;
			#print "input:$input\n";
			if($input =~ m/\s*\[(.*):(.*)\]\s*(.*)\s*;/){	
				my $end = $1;
				my $start = $2;
				my $input_name = $3;
				#print "here!"."$input_name\n";
				push (@verilog_input, $input_name);
                my $size = 0;
                if ($end =~ m/^\d+$/){
				    $size = $end - $start + 1;
                    $size = "".$size;
                } elsif($end =~m/(\S+) - 1/) {
                    $size = $1;
                }
				push(@verilog_input_size, $size);
			}elsif ($input =~ m/\s*(.*)\s*;/){
				my $input_name = $1;
				#print "here!"."$input_name\n";
				push (@verilog_input, $input_name);
				push(@verilog_input_size, "1");
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
                my $size = 0;
                if ($end =~ m/^\d+$/){
				    $size = $end - $start + 1;
                    $size = "".$size;
                } elsif($end =~m/(\S+) - 1/) {
                    $size = $1;
                }
				push(@verilog_output_size, $size);
			}elsif ($output =~ m/\s*(.*)\s*;/){
				my $output_name = $1;
				#print "here!"."$output_name\n";
				push (@verilog_output, $output_name);
				push(@verilog_output_size, "1");
			}
		}
	}


    print("Parameters: ");
    my $param_str = join ' ', @verilog_param;
    print $param_str."\n";

    print("Inputs: ");
    my $in_str = join ' ', @verilog_input;
    print $in_str."\n";
    print("Outputs: ");
    my $out_str = join ' ', @verilog_output;
    print $out_str."\n";
}

#creat scala folder
my $scala_dir = "$dir/../scala";
mkdir $scala_dir unless (-d $scala_dir);

##############################################################################################################################
if(@m_axi_data_widths < 1){
  push(@bus_names, "gmem_dummy");
  push(@m_axi_data_widths, 32);
}

if(not defined($s_axi_data_width)) {
  $s_axi_data_width=32
}

print "Generating BlackBox file ...\n";
for( $i = 0; $i < @m_axi_data_widths; $i = $i + 1 ){
  print "m_axi_data_width_ $bus_names[$i]= $m_axi_data_widths[$i]\n";
}

print "s_axi_data_width = $s_axi_data_width\n";
# should be under scala folder
open BB, ">$scala_dir/$func_name"."_blackbox.scala";

my $blackbox1 = "
package hls_test_c
import Chisel._
import freechips.rocketchip.config.{Parameters, Field}
import freechips.rocketchip.tile._
import freechips.rocketchip.util._

class test_c() extends BlackBox() {
";
$blackbox1 =~ s/test_c/$func_name/g;

# Print parameters
for( $i = 0; $i < @verilog_param; $i = $i + 1 ){
    $blackbox1 .= "val $verilog_param[$i] = $param_val[$i]\n";
}

print BB $blackbox1;


print BB "\tval io = new Bundle {\n";
my $bb_body = "";

# now if the input name does not start with ap, we assume it is an arg
my $ap_return = 0;
my $ap_clk = 0;
my $ap_rst = 0;
my $ap_rst_n = 0;

my @verilog_axi_io = ();

for( $i = 0; $i < @verilog_input; $i = $i + 1 ){
	my $input_name = $verilog_input[$i]; 
	my $input_size = $verilog_input_size[$i];
	if ($input_name =~ m/^ap_clk$/){
		$ap_clk = 1;
	}
	elsif ($input_name =~ m/^ap_rst$/){
		$ap_rst = 1;
	}
	elsif ($input_name =~ m/^ap_rst_n$/){
		$ap_rst_n = 1;
	}
    elsif($input_name =~ m/^(m_axi|s_axi)\S+$/){
        push (@verilog_axi_io, $input_name);
    }

    print BB "\t\tval $input_name = ";
    if ($input_name =~ m/ap_clk(.*)/){
        print BB "Clock\(INPUT\)\n";
    }else{
      print BB "Bits\(INPUT, width = $input_size\)\n";
    }
}

for( $i = 0; $i < @verilog_output; $i = $i + 1 ){

	my $output_name = $verilog_output[$i]; 
	my $output_size = $verilog_output_size[$i];

	if ($output_name =~ m/ap_return(.*)/){
		$ap_return = 1;
	}
    elsif($output_name =~ m/^(m_axi|s_axi)\S+$/){
        push (@verilog_axi_io, $output_name);
    }

	print BB "\t\tval $output_name = ";
	print BB "Bits(OUTPUT, width = $output_size)\n";

}

print BB "\t}\n";
print BB "}\n";

close BB;
##############################################################################################################################
print "Generating Control file ...\n";

open CT, ">$scala_dir/$func_name"."_accel.scala";

#TODO Fix AXI4 params
my $control1 = '
package hls_test_c

import chisel3._
import chisel3.util._

import freechips.rocketchip.config.{Field, Parameters}
import freechips.rocketchip.diplomacy._
import freechips.rocketchip.tilelink._
import freechips.rocketchip.amba.axi4._
import freechips.rocketchip.util._ 
import freechips.rocketchip.subsystem._

class HLStest_cAXI (address: BigInt = 0x20000, beatBytes: Int = 8) (implicit p: Parameters) extends LazyModule {  

  val numInFlight = 8
';

for( $i = 0; $i < @m_axi_data_widths; $i = $i + 1 ){
  $control1 .="
    val node_$bus_names[$i] = AXI4MasterNode(Seq(AXI4MasterPortParameters(
      masters = Seq(AXI4MasterParameters(
        name = \"axil_hub_mem_out_$i\",
        id = IdRange(0, numInFlight),
        aligned = true,
        maxFlight = Some(8)
      )),
      userBits = 0
      )
    ))";
}
$control1 .='
  val slave_node = AXI4SlaveNode(Seq(AXI4SlavePortParameters(
    slaves = Seq(AXI4SlaveParameters(
      address = List(AddressSet(address,0x4000-1)),
      regionType = RegionType.UNCACHED,
      supportsWrite = TransferSizes(1, beatBytes),
      supportsRead = TransferSizes(1, beatBytes),
      interleavedId = Some(0)
    )),
    beatBytes = beatBytes
  )))

  lazy val module = new HLStest_cAXIModule(this)
}

class HLStest_cAXIModule(outer: HLStest_cAXI) extends LazyModuleImp(outer) {  

    //val (out, edge) = outer.node.out(0)
    val (slave_in, slave_edge) = outer.slave_node.in(0)

    val bId = Reg(UInt(32.W))
    val rId = Reg(UInt(32.W))

	val bb = Module(new test_c())
';

for( $i = 0; $i < @m_axi_data_widths; $i = $i + 1 ){
  $control1 .="
    val (out_$bus_names[$i], edge_$bus_names[$i]) = outer.node_$bus_names[$i].out(0)";
}
$control1 .= "\n";
$control1 =~ s/s_axi_data_width/$s_axi_data_width/g;

if ($ap_clk eq 1){
    $control1 .= "\tbb.io.ap_clk := clock\n";
}
if ($ap_rst eq 1){
    $control1 .= "\tbb.io.ap_rst := reset\n";
}
if ($ap_rst_n eq 1){
    $control1 .= "\tbb.io.ap_rst_n := !reset.toBool()  \n";
}

$control1 =~ s/test_c/$func_name/g;
print CT $control1;
#TODO modify accelerator arg!
my $control2 = '
';

# TODO Add support for multiple AXI buses 
# AXI Inputs Signals  
for( $i = 0; $i < @verilog_axi_io; $i = $i + 1 ){
	my $number = $i + 1;
    if ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(AW|W|AR)READY$/){
        my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tbb.io.$verilog_axi_io[$i] := out_$bus_name.$type.ready\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(R|B)VALID$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tbb.io.$verilog_axi_io[$i] := out_$bus_name.$type.valid\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(R)DATA$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tbb.io.$verilog_axi_io[$i] := out_$bus_name.$type.bits.data\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(R)LAST$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tbb.io.$verilog_axi_io[$i] := out_$bus_name.$type.bits.last\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(R|B)ID$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tbb.io.$verilog_axi_io[$i] := out_$bus_name.$type.bits.id\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(R|B)RESP$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tbb.io.$verilog_axi_io[$i] := out_$bus_name.$type.bits.resp\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/s_axi_(.*)_(AW|W|AR)VALID$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tbb.io.$verilog_axi_io[$i] := slave_in.$type.valid\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/s_axi_(.*)_(AW|AR)ADDR$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tbb.io.$verilog_axi_io[$i] := slave_in.$type.bits.addr\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/s_axi_(.*)_(W)DATA$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tbb.io.$verilog_axi_io[$i] := slave_in.$type.bits.data\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/s_axi_(.*)_(W)STRB$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tbb.io.$verilog_axi_io[$i] := slave_in.$type.bits.strb\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/s_axi_(.*)_(R|B)READY$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tbb.io.$verilog_axi_io[$i] := slave_in.$type.ready\n";
    }
}

for( $i = 0; $i < @verilog_axi_io; $i = $i + 1 ){
	  my $number = $i + 1;
    if ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(AW|W|AR)VALID$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tout_$bus_name.$type.valid := bb.io.$verilog_axi_io[$i]\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(R|B)READY$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tout_$bus_name.$type.ready := bb.io.$verilog_axi_io[$i]\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(AW|AR)ADDR$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tout_$bus_name.$type.bits.addr := bb.io.$verilog_axi_io[$i]\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(AW|AR)ID$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tout_$bus_name.$type.bits.id := bb.io.$verilog_axi_io[$i]\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(AW|AR)LEN$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tout_$bus_name.$type.bits.len := bb.io.$verilog_axi_io[$i]\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(AW|AR)SIZE$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tout_$bus_name.$type.bits.size := bb.io.$verilog_axi_io[$i]\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(AW|AR)BURST$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tout_$bus_name.$type.bits.burst := bb.io.$verilog_axi_io[$i]\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(AW|AR)LOCK$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tout_$bus_name.$type.bits.lock := bb.io.$verilog_axi_io[$i]\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(AW|AR)CACHE$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tout_$bus_name.$type.bits.cache := bb.io.$verilog_axi_io[$i]\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(AW|AR)PROT$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tout_$bus_name.$type.bits.prot := bb.io.$verilog_axi_io[$i]\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(AW|AR)QOS$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tout_$bus_name.$type.bits.qos := bb.io.$verilog_axi_io[$i]\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(AW|AR)REGION$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\t//out_$bus_name.$type.bits.region := bb.io.$verilog_axi_io[$i]\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(W)DATA$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tout_$bus_name.$type.bits.data := bb.io.$verilog_axi_io[$i]\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(W)STRB$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tout_$bus_name.$type.bits.strb := bb.io.$verilog_axi_io[$i]\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/m_axi_(.*)_(W)LAST$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tout_$bus_name.$type.bits.last := bb.io.$verilog_axi_io[$i]\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/s_axi_(.*)_(AW|W|AR)READY$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tslave_in.$type.ready := bb.io.$verilog_axi_io[$i]\n";
    }
    elsif ($verilog_axi_io[$i] =~ m/s_axi_(.*)_(R|B)VALID$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tslave_in.$type.valid := bb.io.$verilog_axi_io[$i]\n";
    }
   elsif ($verilog_axi_io[$i] =~ m/s_axi_(.*)_(R)DATA$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tslave_in.$type.bits.data := bb.io.$verilog_axi_io[$i]\n";
    }
   elsif ($verilog_axi_io[$i] =~ m/s_axi_(.*)_(R|B)RESP$/){
				my $bus_name = $1;
        my $type = lc $2; 
        $control2 .= "\tslave_in.$type.bits.resp := bb.io.$verilog_axi_io[$i]\n";
    }
}

if ($ap_return eq 1){
	$control2 = $control2."\tval ap_return = accel.io.ap.rtn\n";
}
$control2 .= "
    // For AXI4lite, these two signals are always True
    slave_in.r.bits.last := true.B 

    when(slave_in.aw.fire()){
        bId := slave_in.aw.bits.id
    }

    when(slave_in.ar.fire()){
        rId := slave_in.ar.bits.id 
    }
    slave_in.r.bits.id := rId
    slave_in.b.bits.id := bId
}
";

# TODO Fix the width here
$control2 .='
trait HasPeripheryHLStest_cAXI { this: BaseSubsystem =>
  private val address = BigInt(base_addr)
  private val axi_m_portName = "HLS-Accelerator-test_c-master"
  private val axilite_s_portName = "HLS-Accelerator-test_c-slave"

  //val accel_s_axi_width = s_axi_data_width 
  //val hls_test_c_accel = LazyModule(new HLStest_cAXI(address, sbus.beatBytes))
  val hls_test_c_accel = LazyModule(new HLStest_cAXI(address, s_axi_data_width >> 3))
';


for( $i = 0; $i < @m_axi_data_widths; $i = $i + 1 ){
  $control2 .="
    sbus.fromPort(Some(axi_m_portName)) {
            (TLWidthWidget($m_axi_data_widths[$i]>> 3 ) 
            := AXI4ToTL() 
            := AXI4UserYanker() 
            := AXI4Fragmenter() 
            := AXI4IdIndexer(1))
    }:=* hls_test_c_accel.node_$bus_names[$i]
    ";
}

$control2 .='
  hls_test_c_accel.slave_node :=* sbus.toFixedWidthPort(Some(axilite_s_portName)) {
                  (AXI4Buffer()    
                       := AXI4UserYanker() 
                       //:= AXI4IdIndexer(params.idBits)
                       //:= AXI4Deinterleaver(sbus.blockBytes) // Assume there is no iterleaved requests, iterleaveId = Some(0) 
                       := TLToAXI4() 
                       := TLBuffer()      
                       //:= TLWidthWidget(s_axi_data_width >> 3)
                       // Compared to TLWidthWidget, TLFragmenter saves the id info?
                       := TLFragmenter(s_axi_data_width >> 3, 64, true))
  }
}

trait HasPeripheryHLStest_cAXIImp extends LazyModuleImp {
  val outer: HasPeripheryHLStest_cAXI
}';

$control2 =~ s/test_c/$func_name/g;
$control2 =~ s/base_addr/$func_base_addr/g;
$control2 =~ s/s_axi_data_width/$s_axi_data_width/g;
print CT $control2;

