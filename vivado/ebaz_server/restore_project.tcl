if { [llength $argv] == 0 } {
    error "Project name must be specified."
}
set project_name [lindex $argv 0]
set part {xc7z010clg400-1}

create_project $project_name .

#set_param board.repoPaths [concat [file join $::env(HOME) {.Xilinx/Vivado/} [version -short] {xhub/board_store}] [get_param board.repoPaths]]
#set_property BOARD_PART_REPO_PATHS [get_param board.repoPaths] [current_project]

set_property PART $part [current_project]

lappend ip_repo_path_list [file normalize ../../mii_mac]
lappend ip_repo_path_list [file normalize ../../mii_axis]
lappend ip_repo_path_list [file normalize ../../arp]
set_property ip_repo_paths $ip_repo_path_list [get_filesets sources_1]
update_ip_catalog

source ./design_1.tcl
make_wrapper -top -fileset sources_1 -import [get_files $project_name.srcs/sources_1/bd/design_1/design_1.bd]

add_files ./constraint.xdc -fileset [get_filesets constrs_1]e