package vivadoHLS

import Chisel._

//These are definitions of the bus standards used on Vivado HLS generated accelerators

//Request Packet Format
class ApBusReq(dataWidth:Int, addrWidth:Int) extends Bundle{
  //Req specific lines
  //Specifies a write request
  val din     = Bool(OUTPUT) //req_din in verilog

  //Lines used for req
  val address     = UInt(OUTPUT, width = addrWidth)
  val dataout     = UInt(OUTPUT, width = dataWidth)
  val size        = UInt(OUTPUT, width = addrWidth)
}

//Response Packet Format
class ApBusRsp(dataWidth:Int) extends Bundle{
  val datain      = UInt(INPUT , width = dataWidth)
}

class ApBusIO(dataWidth:Int = 64, addrWidth:Int = 32) extends Bundle{
  val req = new ApBusReq(dataWidth, addrWidth)
  val req_full_n  = Bool(INPUT ) //req_full_n in verilog
  //Write the request
  val req_write   = Bool(OUTPUT) //req_write in verilog

  val rsp = new ApBusRsp(dataWidth)
  val rsp_empty_n = Bool(INPUT )
  val rsp_read    = Bool(OUTPUT)
}

class ApCtrlIO(dataWidth:Int = 64) extends Bundle{
  //val clk    = Bool(INPUT )
  //val rst    = Bool(INPUT )
  val start  = Bool(INPUT )
  val done   = Bool(OUTPUT)
  val idle   = Bool(OUTPUT)
  val ready  = Bool(OUTPUT)
  val rtn    = UInt(OUTPUT, width = dataWidth)
}
