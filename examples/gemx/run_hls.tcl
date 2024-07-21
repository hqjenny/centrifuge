open_project -reset gemx_tl_prj
set_top gemx
add_files -cflags "-std=c++11"  gemx_tl.cpp
open_solution -reset "solution1"
config_compile -ignore_long_run_time
set_part {xcvu9p-flgb2104-2-i}
create_clock -period 3.333333 -name default

#source "./gemx_tl_prj/solution1/directives.tcl"
#config_interface -clock_enable
config_interface -m_axi_addr64
csynth_design
#export_design -format ip_catalog
exit
