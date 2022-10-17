#!/usr/bin/perl
use warnings;
use strict;
use Cwd;
use File::Copy;

sub generate_config {

    my $rdir = $ENV{'RDIR'};
    print $rdir;
    if ((not defined($rdir)) or $rdir eq '') {
        print("Please source sourceme-f1-manager.sh!\n");
        exit();
    }

    my @rocc_func_names = @{$_[0]};
    my @tll2_func_names = @{$_[1]};

    my $postfix = "";
    my $num_args = scalar @_;
    if ($num_args > 2) {
      $postfix= $_[2];
    }

    my $rocc = "";
    #if (@rocc_func_names > 0) { 
        $rocc .= "
class WithHLSRoCCExample extends Config((site, here, up) => {
  case BuildRoCC => Seq(
";
    #}
    for( my $i = 0; $i < @rocc_func_names; $i = $i + 1 ){
        if ($i ne 0) { $rocc.=",
";}
        $rocc .="
    (p: Parameters) => {
        val hls_$rocc_func_names[$i] = LazyModule(new HLS$rocc_func_names[$i]Control(OpcodeSet.custom$i)(p))
        hls_$rocc_func_names[$i]
    }";
    }

   if (scalar @rocc_func_names ne 0) { $rocc.=",
";}
    $rocc .= "
    (p: Parameters) => {
        val translator = LazyModule(new TranslatorExample(OpcodeSet.custom3)(p))
        translator 
    })";

    #if (@rocc_func_names > 0) { 
        $rocc .= "\n})\n";
    #}
    open CONFIG, ">$rdir/generators/chipyard/src/main/scala/config/HLSConfig.scala" or die "$!\n";
    my $config="package chipyard
import chisel3._
import freechips.rocketchip.diplomacy.{LazyModule, ValName}
import freechips.rocketchip.config.{Parameters, Config}
import testchipip.{WithBlockDevice, BlockDeviceKey, BlockDeviceConfig}
import freechips.rocketchip.tile._
import freechips.rocketchip.subsystem._
import freechips.rocketchip.system.DefaultConfig
import freechips.rocketchip.rocket._
import freechips.rocketchip.tilelink._     
import freechips.rocketchip.devices.tilelink._
import freechips.rocketchip._
import testchipip._
import sifive.blocks.devices.uart.{PeripheryUARTKey,UARTParams}

import sifive.blocks.devices.uart._
import java.io.File
";
    foreach my $func_name (@rocc_func_names) {
        $config .= "import hls_$func_name.HLS$func_name"."Control\n";
    }
    foreach my $func_name (@tll2_func_names) {
        $config .= "import hls_$func_name._\n";
    }

    $config .= $rocc;
    $config .="
class HLSRocketConfig extends Config(
  new WithHLSRoCCExample ++ 
  new freechips.rocketchip.subsystem.WithNBigCores(1) ++
  new chipyard.config.AbstractConfig)
";
    print CONFIG $config;
    close CONFIG;

##############################FireSim Config Generation ##############################
    open CONFIG, ">$rdir/generators/firechip/src/main/scala/HLSConfig.scala" or die "$!\n";
    $config='package firesim.firesim

import chisel3._
import freechips.rocketchip._
import freechips.rocketchip.tile._
import freechips.rocketchip.rocket._
import freechips.rocketchip.subsystem._
import freechips.rocketchip.diplomacy._
import freechips.rocketchip.config.{Parameters, Config}
import freechips.rocketchip.tilelink._
import freechips.rocketchip.devices.tilelink._
import freechips.rocketchip.devices.debug.HasPeripheryDebugModuleImp
import freechips.rocketchip.config.Parameters
import freechips.rocketchip.util.{HeterogeneousBag}
import freechips.rocketchip.amba.axi4.AXI4Bundle
import freechips.rocketchip.config.{Field, Parameters}
import freechips.rocketchip.diplomacy.LazyModule
import utilities.{Subsystem, SubsystemModuleImp}
import icenet._
import firesim.util.DefaultFireSimHarness
import testchipip._
import testchipip.SerialAdapter.SERIAL_IF_WIDTH
import tracegen.{HasTraceGenTiles, HasTraceGenTilesModuleImp}
import sifive.blocks.devices.uart._
import java.io.File

import firesim.bridges._
import firesim.util.{WithNumNodes}
import firesim.configs._

import FireSimValName._
';
    foreach my $func_name (@rocc_func_names) {
        $config .= "import hls_$func_name.HLS$func_name"."Control\n";
    }
    foreach my $func_name (@tll2_func_names) {
        $config .= "import hls_$func_name._\n";
    }

    $config .= $rocc;

$config .="
class HLSFireSimRocketChipConfig$postfix extends Config(
  new WithBootROM ++
  new WithPeripheryBusFrequency(BigInt(3200000000L)) ++
  new WithExtMemSize(0x400000000L) ++ // 16GB
  new WithoutTLMonitors ++
  new WithUARTKey ++
  new WithNICKey ++
  new WithBlockDevice ++
  new WithRocketL2TLBs(1024) ++
  new WithPerfCounters ++
  new WithInclusiveCache ++
  new WithoutClockGating ++
  new WithDefaultMemModel ++
  new WithDefaultFireSimBridges ++
  new WithHLSRoCCExample ++
  new freechips.rocketchip.system.DefaultConfig)
";

    $config .="
class FireSimTopWithHLSDUT(implicit p: Parameters) extends FireSimDUT";

    foreach my $func_name (@tll2_func_names) {
        $config .= "\n    with HasPeripheryHLS$func_name"."AXI";
    }

  $config .=' {
  override lazy val module = new FireSimTopWithHLSModuleImp(this)
}

class FireSimTopWithHLSModuleImp[+L <: FireSimTopWithHLSDUT](l: L) extends FireSimModuleImp(l)
';
    foreach my $func_name (@tll2_func_names) {
        $config .= "    with HasPeripheryHLS$func_name"."AXIImp\n";
    }

    $config .= '
class FireSimTopWithHLS (implicit p: Parameters) extends DefaultFireSimHarness(() => new FireSimTopWithHLSDUT)
';

    $config .="
class FireSimTopWithHLSNoNICDUT(implicit p: Parameters) extends FireSimNoNICDUT";

    foreach my $func_name (@tll2_func_names) {
        $config .= "\n    with HasPeripheryHLS$func_name"."AXI";
    }

  $config .=' {
  override lazy val module = new FireSimTopWithHLSNoNICModuleImp(this)
}

class FireSimTopWithHLSNoNICModuleImp[+L <: FireSimTopWithHLSNoNICDUT](l: L) extends FireSimNoNICModuleImp(l)
';
    foreach my $func_name (@tll2_func_names) {
        $config .= "    with HasPeripheryHLS$func_name"."AXIImp\n";
    }

    $config .= '
class FireSimTopWithHLSNoNIC (implicit p: Parameters) extends DefaultFireSimHarness(() => new FireSimTopWithHLSNoNICDUT)
';

    print CONFIG $config;

    close CONFIG;
} 
1;
