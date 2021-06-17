open_project ethernet_service
set_top ethernet_service
add_files ethernet_service.cpp
add_files ethernet_service.hpp
add_files -tb test.cpp -cflags "-Wno-unknown-pragmas"
open_solution "solution1" -flow_target vivado
set_part {xc7z010-clg400-1}
create_clock -period 40 -name default
config_export -description {Ethernet Service} -display_name ethernet_service -format ip_catalog -library Network -output ip/ethernet_service.zip -rtl verilog -vendor fugafuga.org -version 1.0
# csim_design
csynth_design
# cosim_design
export_design -rtl verilog -format ip_catalog -description "Ethernet Service" -vendor "fugafuga.org" -library "Network" -display_name "ethernet_service" -output ip/ethernet_service.zip
