open_project -reset vgg_tl_prj
set_top vgg
add_files vgg_tl.cpp -cflags "-std=c++0x" 
open_solution -reset "solution1"
set_part {xcvu9p-flgb2104-2-i}
config_compile -ignore_long_run_time
create_clock -period 10 -name default
#source "./vgg_tl_prj/solution1/directives.tcl"
#config_interface -clock_enable
config_interface -m_axi_addr64
csynth_design
#export_design -format ip_catalog
exit