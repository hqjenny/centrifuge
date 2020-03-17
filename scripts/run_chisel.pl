#!/usr/bin/perl
use warnings;
use strict;
use Cwd;
use File::Copy;
use List::Util qw(first);

my $dir = getcwd;
my $file_name = $ARGV[0];
my $func_name = $ARGV[1];
my $rdir = $ENV{'RDIR'};

my $prefix = undef;

my $num_args = $#ARGV + 1;
if ($num_args > 2) {
  $prefix = $ARGV[2];
}

if ($prefix) {
  $func_name  = $prefix.$func_name;
}

#print $rdir;
if ((not defined($rdir)) or $rdir eq '') {
    print("Please source sourceme-f1.sh!\n");
    exit();
}


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
open BB, ">$scala_dir/$func_name"."_blackbox.scala";

my $blackbox1 = "
package hls_test_c
import Chisel._
import chisel3.experimental.dontTouch
import freechips.rocketchip.config.{Parameters, Field}
import freechips.rocketchip.tile._
import freechips.rocketchip.util._
import vivadoHLS._

class test_c() extends BlackBox() {
";
$blackbox1 =~ s/test_c/$func_name/g;

print BB $blackbox1;

print BB "\tval io = new Bundle {\n";
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
	elsif(!($input_name =~ m/ap_(.*)/)){
		push (@verilog_input_scalar, $input_name);
    push(@sindices, $arg_count);
    $arg_count = $arg_count + 1;
	} 
    else{
        print("Not func args: $input_name\n");
    }

	print BB "\t\tval $input_name = ";
  if ($input_name =~ m/ap_clk(.*)/){
		print BB "Clock\(INPUT\)\n";
  }else{
    if ($input_size == 1){
      print BB "Bool\(INPUT\)\n";
    }else{
      print BB "Bits\(INPUT, width = $input_size\)\n";
    }
  }
	if($input_name ne "ap_clk" && $input_name ne "ap_rst"){
		$bb_body = $bb_body."\tio.".$input_name.".setName(\"".$input_name."\")\n";
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

	print BB "\t\tval $output_name = ";
	if ($output_size == 1){
		print BB "Bool(OUTPUT)\n";
	}else{
		print BB "Bits(OUTPUT, width = $output_size)\n";
	}

	$bb_body = $bb_body."\tio.".$output_name.".setName(\"".$output_name."\")\n";
}

if ($ap_clk eq 1){
	$bb_body = $bb_body."addClock(Driver\.implicitClock)\n".'renameClock("clk", "ap_clk")'."\n";
}

if ($ap_rst eq 1){
	$bb_body = $bb_body.'renameReset("ap_rst")'."\n";
}

print BB "\t}\n";
#print BB "$bb_body\n";
#print BB "moduleName = "."\"$func_name\"\n";
print BB "}\n";

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

$bb_def .= "\tval ap_bus_addrWidths = List(".$addr_widths_str.")\n";
$bb_def .= "\tval ap_bus_dataWidths = List(".$data_widths_str.")\n";

#$bb_def .= "\tval ap_bus_argLoc = List(".$indices_str.")\n";
$bb_def .= "\tval ap_bus_argLoc = List(".$pindices_str.")\n";

my $ret_width = 1;
if ($ap_return eq 1){
    my $ret_idx = first { $verilog_output[$_] eq 'ap_return'} 0..$#verilog_output;  
    $ret_width =  $verilog_output_size[$ret_idx]; 
}

$bb_def .= "\tval io = new Bundle {
\tval ap = new ApCtrlIO(dataWidth = $ret_width)
\tval ap_bus = HeterogeneousBag(ap_bus_addrWidths.zip(ap_bus_dataWidths).map {
      case (aw, dw) => new ApBusIO(dw, aw)
    })
";

if (@verilog_input_scalar > 0){
    $bb_def .="\tval scalar_io = HeterogeneousBag(scalar_io_dataWidths.map(w => UInt(INPUT, width = w)))";
}
$bb_def .="
}

\tval bb = Module(new $func_name())

\tbb.io.ap_start := io.ap.start
\tio.ap.done := bb.io.ap_done
\tio.ap.idle := bb.io.ap_idle
\tio.ap.ready := bb.io.ap_ready
";

if ($ap_return eq 1) {
    $bb_def .= "\tio.ap.rtn := bb.io.ap_return\n";
}

if ($ap_rst eq 1) {
   $bb_def .=  "\tbb.io.ap_rst := reset\n";
} 
if ($ap_clk eq 1) {
   $bb_def .=  "\tbb.io.ap_clk := clock\n";
} 

$idx = 0;
#foreach my $arg (keys %verilog_input_pointer) {
foreach my $arg (@verilog_input_pointer_arg) {
   $bb_def.="\tio.ap_bus($idx).req.din := bb.io.$arg"."_req_din 
\tbb.io.$arg"."_req_full_n := io.ap_bus($idx).req_full_n 
\tio.ap_bus($idx).req_write := bb.io.$arg"."_req_write
\tbb.io.$arg"."_rsp_empty_n := io.ap_bus($idx).rsp_empty_n
\tio.ap_bus($idx).rsp_read := bb.io.$arg"."_rsp_read
\tio.ap_bus($idx).req.address := bb.io.$arg"."_address
\tbb.io.$arg"."_datain := io.ap_bus($idx).rsp.datain
\tio.ap_bus($idx).req.dataout := bb.io.$arg"."_dataout
\tio.ap_bus($idx).req.size := bb.io.$arg"."_size
";
    $idx += 1;
}
$idx = 0;
foreach my $arg (@verilog_input_scalar) {   
    $bb_def .="\tbb.io.$arg := io.scalar_io($idx)\n";
    $idx += 1;
}

$bb_def .= "}";

print BB $bb_def;
close BB;

##############################################################################################################################
print "Copying Vivado HLS Interface file ...\n";
copy("$rdir/tools/centrifuge/scripts/chisel_rocc_aux/ap_bus.scala", "$scala_dir/") or die "Copy failed: $!";

##############################################################################################################################
print "Copying ROCC  Memory Controller file ...\n";
copy("$rdir/tools/centrifuge/scripts/chisel_rocc_aux/memControllerComponents.scala", "$scala_dir/") or die "Copy failed: $!";

##############################################################################################################################
print "Copying Controller Utilities file ...\n";
copy("$rdir/tools/centrifuge/scripts/chisel_rocc_aux/controlUtils.scala", "$scala_dir/") or die "Copy failed: $!";

##############################################################################################################################
print "Generating Control file ...\n";

open CT, ">$scala_dir/$func_name"."_accel.scala";
my $control1 = '
package hls_test_c
import Chisel._
import chisel3.experimental.dontTouch
import freechips.rocketchip.config.{Parameters, Field}
import freechips.rocketchip.tile._
import freechips.rocketchip.config._
import freechips.rocketchip.diplomacy._
import freechips.rocketchip.rocket._
import freechips.rocketchip.tilelink._
import freechips.rocketchip.util._
import freechips.rocketchip.system._

import vivadoHLS._
import memControl._
import hls_test_c._

class HLStest_cControl(opcodes: OpcodeSet)(implicit p: Parameters) extends LazyRoCC(opcodes) {
  override lazy val module = new HLStest_cControlModule(this)
}

class HLStest_cControlModule(outer: HLStest_cControl)(implicit p: Parameters) extends LazyRoCCModuleImp(outer) 
    with HasCoreParameters {
';

$control1 =~ s/test_c/$func_name/g;
print CT $control1;
#TODO modify accelerator arg!
my $control2 = '
val result = Reg(init=Bits(0, width=xLen))
val respValid = Reg(init=Bool(false))
val rdy = Reg(init=Bool(true))
val busy = Reg(init=Bool(false))
val bufferedCmd = Reg(init=Wire( new RoCCCommand()(p)))

val cmd = Queue(io.cmd)
val funct = bufferedCmd.inst.funct
val rs1 = bufferedCmd.rs1
val rs2 = bufferedCmd.rs2
val rdTag = bufferedCmd.inst.rd
val doAdd = funct === UInt(0)

val rs1_unbuffered = cmd.bits.rs1
val rs2_unbuffered = cmd.bits.rs2

val idle :: working :: Nil = Enum(UInt(),2)
val state = Reg(init=idle)

when(reset.toBool){
  bufferedCmd.inst.funct := 0.asUInt(7.W)
  bufferedCmd.inst.rs1 := 0.asUInt(5.W)
  bufferedCmd.inst.rs2 := 0.asUInt(5.W)
  bufferedCmd.inst.rd := 0.asUInt(5.W)
  bufferedCmd.inst.opcode := 0.asUInt(5.W)
  bufferedCmd.rs1 := 0.asUInt(64.W)
  bufferedCmd.rs2 := 0.asUInt(64.W)
}

// Assign Outputs to Appropriate registers
io.resp.valid := respValid && bufferedCmd.inst.xd

//need to set rd to the value in the request. Otherwise bad things happen
//in this case, processor stalls
io.resp.bits.rd := rdTag
io.resp.bits.data := result
io.busy := busy
cmd.ready := rdy

//===== Begin Accelerator =====
val accel = Module(new HLStest_cBlackbox())

//Acclerator Registers (we buffer inputs to accelerator)
val ap_start = Reg(init=Bool(false))

//Assign Inputs to Accelerator
accel.io.ap.start := ap_start
';
#accel.io.test_c_rs1 := rs1 //ACCEL IO NAME CAN CHANGE (NAMED IN C)
#accel.io.test_c_rs2 := rs2 //ACCEL IO NAME CAN CHANGE (NAMED IN C)
#my $rs1 = $verilog_input_scalar[0];
#my $rs2 = $verilog_input_scalar[1];

for( $i = 0; $i < @verilog_input_scalar; $i = $i + 1 ){
	my $number = $i + 1;
	$control2 = $control2."accel.io.scalar_io($i) := rs$number\n";
}

if ($ap_return eq 1){
	$control2 = $control2."val ap_return = accel.io.ap.rtn\n";
}else{
	$control2 = $control2."val ap_return = UInt(4)\n";
}


$control2 = $control2.'//Accelerator Outputs
val ap_done = accel.io.ap.done
val ap_idle = accel.io.ap.idle
val ap_ready = accel.io.ap.ready

//===== End Accelerator =====

//===== Begin Mem Controller =====
//The following are specific to the accelerator.  They set the address and data widths of the ap_bus interfaces
val dataWidth = accel.ap_bus_dataWidths
val addrWidth = accel.ap_bus_addrWidths
val reqBufferLen = 4
val rspBufferLen = 4
val maxReqBytes = xLen/8
val roccAddrWidth = coreMaxAddrBits
val roccDataWidth = coreDataBits
val roccTagWidth = coreDCacheReqTagBits
val roccCmdWidth = M_SZ
val roccTypWidth = log2Ceil(coreDataBytes.log2 + 1) 
//val numTags = p(RoccMaxTaggedMemXacts)
val numTags = 16
val tagOffset = 0 //Used if multiple accelerators to avoid tag collisions

//Instantiate Controller
val memControl = Module(new MemController(dataWidth, addrWidth, reqBufferLen, rspBufferLen, maxReqBytes, roccAddrWidth, roccDataWidth, roccTagWidth, roccCmdWidth, roccTypWidth, numTags, tagOffset))

if(accel.io.ap_bus.length > 0){
//We have memory bus interfaces on the accelerator, create a memory controller

//Hook up controller
for(i <- 0 until accel.io.ap_bus.length){
    //memControl.io.reqsIn(i) <> accel.io.ap_bus(i).req
    memControl.io.reqsIn(i) := accel.io.ap_bus(i).req

    accel.io.ap_bus(i).req_full_n := memControl.io.reqsFullN(i)

    memControl.io.reqsWrite(i) := accel.io.ap_bus(i).req_write

    accel.io.ap_bus(i).rsp.datain  := memControl.io.rspOut(i).datain
    accel.io.ap_bus(i).rsp_empty_n := memControl.io.rsp_empty_n(i)
    memControl.io.rsp_read(i)      := accel.io.ap_bus(i).rsp_read
}
io.mem.req.bits.addr       := memControl.io.roCCReqAddr
io.mem.req.bits.tag        := memControl.io.roCCReqTag 
io.mem.req.bits.cmd        := memControl.io.roCCReqCmd 
io.mem.req.bits.size        := memControl.io.roCCReqTyp

// If the address is not a mulitple of 8 byte which the coreDataBits width, 
// We have to shift the N-bit data to the right place in a 64-bit word   
val shift = (memControl.io.roCCReqAddr & UInt( log2Up(coreDataBits) - 1 )) << UInt(3)
io.mem.req.bits.data       := memControl.io.roCCReqData << shift(7,0)
io.mem.req.valid           := memControl.io.roCCReqValid
memControl.io.roCCReqRdy   := io.mem.req.ready
//io.mem.req.bits.phys := Bool(true)

//val roCCRespAddr  = UInt(INPUT, width = roccAddrWidth) // coreMaxAddrBits)
memControl.io.roCCRspTag   := io.mem.resp.bits.tag 
memControl.io.roCCRspCmd   := io.mem.resp.bits.cmd
memControl.io.roCCRspData  := io.mem.resp.bits.data
//val roCCRespTyp   
memControl.io.roCCRspValid := io.mem.resp.valid
}

//===== End Mem Controller =====
';
# The sequence of arg 1 and 2 depends on the sequence they show up in the verilog file
# TODO think about a better way to add this

$control2 .= '//===== Begin Argument Handling =====
//TODO: currently only works for 2 argument calls.  Generalize

val cArgs = List(rs1, rs2)
val cArgsUnbuffered = List(rs1_unbuffered, rs2_unbuffered)
//Argument numbers are specified in the blackbox
';

if (@verilog_input_scalar > 0){
    $control2 .= '//Scalar values
for(i <- 0 until accel.io.scalar_io.length){
accel.io.scalar_io(i) := cArgs(accel.scalar_io_argLoc(i))
}
';
}

$control2 .= '//ap_bus offsets
for(i <- 0 until memControl.io.offsetAddrs.length){
//ap_bus uses the unbuffered input because it is buffered on the first cycle
memControl.io.offsetAddrs(i) := cArgsUnbuffered(accel.ap_bus_argLoc(i))
}
//===== End Argument Handling =====
';


$control2 .='
if(accel.io.ap_bus.length > 0){
//Will run ap_start after offsets loaded
for(i <- 0 until memControl.io.loadOffsets.length){
  memControl.io.loadOffsets(i) := (state === idle) && cmd.fire()
}
}


//===== Begin Controller State Machine Logic =====
switch(state){
is (idle){
  //Waiting for command

  when(cmd.fire()){
    //We have a valid, unserviced command. This code takes ready low so
    //we should not accedently cause an infinite loop

    bufferedCmd := cmd.bits //Accelerator takes from bufferedCmd directly
    busy := Bool(true)
    rdy := Bool(false)
   
    //Load the offsets
    /*if(accel.io.ap_bus.length > 0){
      //Will run ap_start after offsets loaded
      for(i <- 0 until memControl.io.loadOffsets.length){
        memControl.io.loadOffsets(i) := Bool(true)
      }
    }*/

    ap_start := Bool(true)        //Set next state
    state := working
    //Note: Based on timing diagram in Vivado HLS user guide (pg 157), read occurs
    //AFTER the 1st cycle.  There will be a 1 cycle delay before input read as
    //ap_start will be seen on next cycle.  Idealy, ap_start would be raised 1 cycle
    //earlier (ie. not using a register) or it would read the input immediatly
    //when ap_start is raised (I assume this is due to an internal state machine).
    //However, this would ruin the sequential nature of the state machine.  It is
    //possible to save a cycle by assigning ap_start as cmd.valid && state===idle 
    //&& !returned which would be asyncronous and probably trigger 1 cycle earlier.
    //There would be more stringent timing requirements in this case though as the
    //result would need to propogate before the next posEdge of the clk.
    

  }
  when(respValid && io.resp.ready){
    //The processor has read the response.  There is no more data for it
    //Drive resp.valid low to avoid stalling processor
    respValid := Bool(false)
  }
}
is (working){

  //Stop Loading offsets
  /*if(accel.io.ap_bus.length > 0){
    //Will run ap_start after offsets loaded
    for(i <- 0 until memControl.io.loadOffsets.length){
      memControl.io.loadOffsets(i) := Bool(false)
    }
  }*/

  //Waiting for accelerator to finish

  //All of the conditionals below can occure simultaniously
  //and should be kept as seperart when statements
  when(ap_done){
    //The accelerator has completed operation (user guidepg 156) and has
    //has optionally generated a result (not not all accelerators will
    //generated a result.  This is technically not the same as ap_idle
    //which signals when the accelerator is no longer busy. It is actually
    //ap_ready actually determines when the accelerator is ready to accept 
    //more inputs.  This is important for accelerators that do not operate 
    //in a syncronous mode.  This is not true for the types of accelerators 
    //we are creating.

    result := ap_return
    respValid := Bool(true)
  }
  when(ap_ready){
    //The accelerator has read the inputs and is ready to accept new ones.
    //According to the timing diagram,
    //ap_start should be deasserted for the next posedge.
    ap_start := Bool(false)
  }


  if(accel.io.ap_bus.length == 0){
    when(ap_idle){
      //if the operation was completed (result valid), and the accelerator is ready
      //the accerator is ready for the next operation and the controller is
      //returned to the idle state to wait for a new command.  the ready line
      //is pulled high to advertise that the accelerator is ready.
      
      //from the manual, it appears that ap_done is always asserted when the
      //accelerator is finished.  if this is true, using ap_done && ap_ready
      //should save one cycle over using ap_idle as the trigger.  this is because,
      //according to the timing diagram in the user manual (pg  

      rdy := Bool(true) // ready to accept new commands
      busy := Bool(false) // operation complete, no longer busy
      state := idle

      //note: this code could possibly be placed in the ap_done action to save
      //one wasted cycle.  it is not clear from the user guide (pg 157), ap_idle
      //is asserted one cycle after ap_done.  if this arrangment has problems,
      //transitioning on ap_idle should work but will result in an unnessicary
      //extra cycle.
    }
  }
  else
  {
    when(ap_idle && !memControl.io.memBusy){
      //if the operation was completed (result valid), and the accelerator is ready
      //the accerator is ready for the next operation and the controller is
      //returned to the idle state to wait for a new command.  the ready line
      //is pulled high to advertise that the accelerator is ready.
      
      //from the manual, it appears that ap_done is always asserted when the
      //accelerator is finished.  if this is true, using ap_done && ap_ready
      //should save one cycle over using ap_idle as the trigger.  this is because,
      //according to the timing diagram in the user manual (pg  

      rdy := Bool(true) // ready to accept new commands
      busy := Bool(false) // operation complete, no longer busy
      state := idle

      //note: this code could possibly be placed in the ap_done action to save
      //one wasted cycle.  it is not clear from the user guide (pg 157), ap_idle
      //is asserted one cycle after ap_done.  if this arrangment has problems,
      //transitioning on ap_idle should work but will result in an unnessicary
      //extra cycle.
    }
  }
  when(respValid && io.resp.ready){
    //The processor has read the response.  There is no more data for it
    //Drive resp.valid low to avoid stalling processor
    respValid := Bool(false)
  }
}
}

// ===== End Controller State Machine Logic =====

// ===== Tie off these lines =====
  io.interrupt := Bool(false)
  // Set this true to trigger an interrupt on the processor (please refer to supervisor documentation)

  // MEMORY REQUEST INTERFACE
  if(accel.io.ap_bus.length == 0){
    // No connected memory bus lines on accelerator
    // We will not be doing any memory ops in this accelerator
    io.mem.req.valid := Bool(false)
    io.mem.req.bits.addr := UInt(0)
    io.mem.req.bits.tag := UInt(0)
    io.mem.req.bits.cmd := M_XRD // perform a load (M_XWR for stores)
    io.mem.req.bits.size := log2Ceil(8).U
    io.mem.req.bits.signed := Bool(false)
    io.mem.req.bits.data := UInt(0) // not performing any stores
  }
  //io.mem.invalidate_lr := Bool(false)

  //If enable physical addr, make sure to use pmp instr to set the right permission on addr range
  io.mem.req.bits.phys := Bool(false)
';


$control2 .= "}\n";
# TODO no clock and reset signal
$control2 =~ s/test_c/$func_name/g;

print CT $control2;


