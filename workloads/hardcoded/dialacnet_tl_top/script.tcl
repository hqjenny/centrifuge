open_project -reset dialacnet_tl_prj
set_top top
open_solution -reset "solution1"
add_files dialacnet_tl.cpp -cflags "-std=c++0x -I../ -g" 
add_files -tb main_tb.cpp -cflags "-std=c++0x -I../ -g" 

#set_part {xczu3eg-sbva484-1-i}
#set_part {xc7z020clg400-1}
set_part {xcvu9p-flgb2104-2-i}
create_clock -period 3 -name default
#config_compile -unsafe_math_optimizations
#config_schedule -relax_ii_for_timing
config_interface   -m_axi_addr64  
csim_design -compiler gcc 
csynth_design
cosim_design -compiler gcc -trace_level all 
export_design -flow syn -rtl verilog -format ip_catalog
exit
