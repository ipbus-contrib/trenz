
set_property CONFIG_VOLTAGE 3.3 [current_design]
set_property CFGBVS VCCO [current_design]

set_property BITSTREAM.Config.SPI_BUSWIDTH 4 [current_design]
set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]

# Ethernet RefClk (125MHz)
create_clock -period 8.000 -name eth_refclk [get_ports eth_clk_p]

# Ethernet monitor clock hack (62.5MHz)
create_clock -period 16.000 -name clk_dc [get_pins infra/eth/dc_buf/O]

#--
# IPbus clock
create_generated_clock -name ipbus_clk -source [get_pins infra/clocks/mmcm/CLKIN1] [get_pins infra/clocks/mmcm/CLKOUT1]

# 200 Mhz derived clock
create_generated_clock -name clk_200 -source [get_pins infra/clocks/mmcm/CLKIN1] [get_pins infra/clocks/mmcm/CLKOUT3]

# Clock constraints
set_false_path -through [get_pins infra/clocks/rst_reg/Q]
set_false_path -through [get_nets infra/clocks/nuke_i]
#--

set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks eth_refclk] -group [get_clocks -include_generated_clocks [get_clocks -filter {name =~ infra/eth/phy/*/RXOUTCLK}]] -group [get_clocks -include_generated_clocks [get_clocks -filter {name =~ infra/eth/phy/*/TXOUTCLK}]]

# MGT_CLK_0_P/N
set_property PACKAGE_PIN F6 [get_ports eth_clk_p]
set_property PACKAGE_PIN E6 [get_ports eth_clk_n]

# SFP for IPBUS control. SFP0 in MIB schematic.
set_property IOSTANDARD LVCMOS25 [get_ports {eth_sfp_los eth_sfp_tx_dis}]
# SFP.LOS0, B16_L10_N
set_property PACKAGE_PIN A14 [get_ports {eth_sfp_los}]
# SFP.TX_DISABLE0, B16_L11_P
set_property PACKAGE_PIN B17 [get_ports {eth_sfp_tx_dis}]
false_path {eth_sfp_los} eth_refclk

# SFP.RD_N0, MGT_TX0_N
set_property PACKAGE_PIN A4 [get_ports {eth_tx_n}]
# SFP.RD_P0, MGT_TX0_P
set_property PACKAGE_PIN B4 [get_ports {eth_tx_p}]

# SFP.TD_N0, MGT_RX0_N
set_property PACKAGE_PIN A8 [get_ports {eth_rx_n}]
# SFP.TD_P0, MGT_RX0_P
set_property PACKAGE_PIN B8 [get_ports {eth_rx_p}]

#-------------------- GPIO --------------------
#gpio_p/n[0] bank 14 at 3v3 for UART.
set_property IOSTANDARD LVCMOS33 [get_ports {gpio_uart_rd  gpio_uart_td}]

#gpio_p/n[1] bank 16 at 2v5 for LVDS Clk monitoring.
set checkPorts [get_ports * -filter {DIRECTION == OUT && NAME =~ "gpio_clk_mon*" }]
if { [llength $checkPorts] != 0} {
set_property IOSTANDARD LVDS_25 [get_ports {gpio_clk_mon_p gpio_clk_mon_n}]
# GPIO1 B16_L3_P
set_property PACKAGE_PIN C14 [get_ports {gpio_clk_mon_p}]
# GPIO0 B16_L3_N
set_property PACKAGE_PIN C15 [get_ports {gpio_clk_mon_n}]
}

# GPIO0 B16_L14_P
#set_property PACKAGE_PIN E19 [get_ports {gpio_p[0]}]
# GPIO2 B16_L12_P
#set_property PACKAGE_PIN D17 [get_ports {gpio_p[2]}]
# GPIO3 B16_L8_P
#set_property PACKAGE_PIN C13 [get_ports {gpio_p[3]}]
# GPIO4 B14_L18_P
#set_property PACKAGE_PIN U17 [get_ports {gpio_p[4]}]
# GPIO5 B14_L9_P
#set_property PACKAGE_PIN Y21 [get_ports {gpio_p[5]}]
# GPIO6 B14_L4_P
set_property PACKAGE_PIN T21 [get_ports {gpio_uart_rd}]
# GPIO7 B16_L4_N
#set_property PACKAGE_PIN E14 [get_ports {gpio_p[7]}]

# GPIO0 B16_L14_N
#set_property PACKAGE_PIN D19 [get_ports {gpio_n[0]}]
# GPIO0 B16_L12_N
#set_property PACKAGE_PIN C17 [get_ports {gpio_n[2]}]
# GPIO0 B16_L8_N
#set_property PACKAGE_PIN B13 [get_ports {gpio_n[3]}]
# GPIO0 B14_L18_N
#set_property PACKAGE_PIN U18 [get_ports {gpio_n[4]}]
# GPIO0 B14_L9_N
#set_property PACKAGE_PIN Y22 [get_ports {gpio_n[5]}]
# GPIO0 B14_L4_N
set_property PACKAGE_PIN U21 [get_ports {gpio_uart_td }]
# GPIO0 B16_L14_P
#set_property PACKAGE_PIN E13 [get_ports {gpio_n[7]}]

#-------------------- I2C --------------------
# The I2C lines are selected by an I2C switch.
set_property IOSTANDARD LVCMOS25 [get_ports {fpga_i2c_*}]
# B15_L20_N, FPGA_I2C_SCL
set_property PACKAGE_PIN L13 [get_ports {fpga_i2c_scl}] 
# B15_L20_P, FPGA_I2C_SDA
set_property PACKAGE_PIN M13 [get_ports {fpga_i2c_sda}]
false_path {fpga_i2c_*} eth_refclk

set_property IOSTANDARD LVCMOS25 [get_ports {rstb_i2c}]
# B15_L24_N, I2CSW_RST
set_property PACKAGE_PIN M16 [get_ports {rstb_i2c}]

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
