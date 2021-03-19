
set_property CONFIG_VOLTAGE 3.3 [current_design]
set_property CFGBVS VCCO [current_design]

set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]

# Ethernet RefClk (125MHz)
create_clock -period 8.000 -name eth_refclk [get_ports eth_clk_p]

# Ethernet monitor clock hack (62.5MHz)
create_clock -period 16.000 -name clk_dc [get_pins infra/eth/dc_buf/O]

# System synchronous clock (40MHz nominal)
create_clock -period 25.000 -name clk40 [get_ports clk_p]

set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks eth_refclk] -group [get_clocks -include_generated_clocks [get_clocks -filter {name =~ infra/eth/phy/*/RXOUTCLK}]] -group [get_clocks -include_generated_clocks [get_clocks -filter {name =~ infra/eth/phy/*/TXOUTCLK}]]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks clk40]

# Area constraints
#create_pblock infra
#resize_pblock [get_pblocks infra] -add {CLOCKREGION_X1Y4:CLOCKREGION_X1Y4}

set_property PACKAGE_PIN F6 [get_ports eth_clk_p]
set_property PACKAGE_PIN E6 [get_ports eth_clk_n]

set_property LOC GTPE2_CHANNEL_X0Y4 [get_cells -hier -filter {name=~infra/eth/*/gtpe2_i}]
set_property LOC GTPE2_CHANNEL_X0Y6 [get_cells -hier -filter {name=~*/mgt_ds/*/gtpe2_i}]
set_property LOC GTPE2_CHANNEL_X0Y7 [get_cells -hier -filter {name=~*/mgt_us/*/gtpe2_i}]

proc false_path {patt clk} {
    set p [get_ports -quiet $patt -filter {direction != out}]
    if {[llength $p] != 0} {
        set_input_delay 0 -clock [get_clocks $clk] [get_ports $patt -filter {direction != out}]
        set_false_path -from [get_ports $patt -filter {direction != out}]
    }
    set p [get_ports -quiet $patt -filter {direction != in}]
    if {[llength $p] != 0} {
       	set_output_delay 0 -clock [get_clocks $clk] [get_ports $patt -filter {direction != in}]
	    set_false_path -to [get_ports $patt -filter {direction != in}]
	}
}

#
# Location of main I2C
# MIB J8 
# SDA = B15_L20_P = T20
# SCL = B15_L20_N = W21
set_property PACKAGE_PIN W21 [get_ports scl]
set_property PACKAGE_PIN T20 [get_ports sda]
