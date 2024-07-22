package firesim.firesim

import java.io.File

import chisel3._
import chisel3.util.{log2Up}
import org.chipsalliance.cde.config.{Parameters, Config}
import freechips.rocketchip.groundtest.TraceGenParams
import freechips.rocketchip.tile._
import freechips.rocketchip.tilelink._
import freechips.rocketchip.rocket.DCacheParams
import freechips.rocketchip.subsystem._
import freechips.rocketchip.devices.tilelink.BootROMParams
import freechips.rocketchip.devices.debug.{DebugModuleParams, DebugModuleKey}
import freechips.rocketchip.diplomacy.LazyModule
import boom.common.BoomTilesKey
import testchipip.{BlockDeviceKey, BlockDeviceConfig, SerialKey, TracePortKey, TracePortParams}
import sifive.blocks.devices.uart.{PeripheryUARTKey, UARTParams}
import scala.math.{min, max}
import tracegen.TraceGenKey
import icenet._
import ariane.ArianeTilesKey
import testchipip.WithRingSystemBus

import firesim.bridges._
import firesim.configs._
import chipyard.{BuildTop}
import chipyard.config.ConfigValName._

class WithHLSFireSimConfigTweaks extends Config(
  new WithBootROM ++ // needed to support FireSim-as-top
  new WithPeripheryBusFrequency(BigInt(3200000000L)) ++ // 3.2 GHz 
  new WithoutClockGating ++
  new WithTraceIO ++
  new freechips.rocketchip.subsystem.WithExtMemSize((1 << 30) * 16L) ++ // 16 GB
  new testchipip.WithTSI ++
  new testchipip.WithBlockDevice ++
  new chipyard.config.WithUART
)

class HLSFireSimRocketChipConfig extends Config(
    new WithDefaultFireSimBridges ++
    new WithDefaultMemModel ++
    new WithHLSFireSimConfigTweaks ++
    new chipyard.HLSRocketConfig
)

