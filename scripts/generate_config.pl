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

    open CONFIG, ">$rdir/generators/example/src/main/scala/HLSConfig.scala" or die "$!\n";
    my $config="package example
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
import ConfigValName._
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
  new WithHLSTop ++
  new WithBootROM ++
  new freechips.rocketchip.subsystem.WithInclusiveCache ++
  new WithHLSRoCCExample ++ 
  new freechips.rocketchip.subsystem.WithNBigCores(1) ++
  new freechips.rocketchip.system.BaseConfig)
";

    $config .="

class WithHLSTop extends Config((site, here, up) => {
  case BuildTop => (clock: Clock, reset: Bool, p: Parameters) =>
      Module(LazyModule(new TopWithHLS()(p)).module)
      })

class TopWithHLS(implicit p: Parameters) extends Top ";

    foreach my $func_name (@tll2_func_names) {
        $config .= "\n    with HasPeripheryHLS$func_name"."AXI";
    }

  $config .=' {
  override lazy val module = new TopWithHLSModule(this)
}

class TopWithHLSModule(l: TopWithHLS) extends TopModule(l)
';
    foreach my $func_name (@tll2_func_names) {
        $config .= "    with HasPeripheryHLS$func_name"."AXIImp\n";
    }

    print CONFIG $config;

    close CONFIG;
} 
1;
