---------------------------------------------------------------------------------
--
--   Copyright 2017 - Rutherford Appleton Laboratory and University of Bristol
--
--   Licensed under the Apache License, Version 2.0 (the "License");
--   you may not use this file except in compliance with the License.
--   You may obtain a copy of the License at
--
--       http://www.apache.org/licenses/LICENSE-2.0
--
--   Unless required by applicable law or agreed to in writing, software
--   distributed under the License is distributed on an "AS IS" BASIS,
--   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--   See the License for the specific language governing permissions and
--   limitations under the License.
--
--                                     - - -
--
--   Additional information about ipbus-firmare and the list of ipbus-firmware
--   contacts are available at
--
--       https://ipbus.web.cern.ch/ipbus
--
---------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

use work.ipbus.ALL;

entity top is port(

        eth_clk_p           : in std_logic;  -- 125MHz MGT clock
        eth_clk_n           : in std_logic;
        eth_rx_p            : in std_logic;  -- Ethernet MGT input
        eth_rx_n            : in std_logic;
        eth_tx_p            : out std_logic;  -- Ethernet MGT output
        eth_tx_n            : out std_logic;
        eth_sfp_los         : in std_logic;
        eth_sfp_tx_dis      : out std_logic;
        fpga_i2c_scl        : inout std_logic;
        fpga_i2c_sda        : inout std_logic;
        rstb_i2c            : out std_logic := '1'; -- Active low rst
        --gpio_clk_mon_n      : out std_logic := '0';
        --gpio_clk_mon_p      : out std_logic := '0';
        gpio_uart_td        : out std_logic;
        gpio_uart_rd        : in std_logic
    );

end top;

architecture rtl of top is

    signal clk_ipb : std_logic; 
    signal rst_ipb, nuke, soft_rst : std_logic;
    signal mac_addr: std_logic_vector(47 downto 0);
    signal ip_addr: std_logic_vector(31 downto 0);
    signal ipb_out: ipb_wbus;
    signal ipb_in: ipb_rbus;
    signal neo430_fpga_i2c_scl_o, neo430_fpga_i2c_sda_o   : std_logic := '1';
    
begin

-- Infrastructure
    eth_sfp_tx_dis <= '0';
    
-- Tri-state I2C lines:
    fpga_i2c_sda <= '0'  when (neo430_fpga_i2c_sda_o= '0') else 'Z';
    fpga_i2c_scl <= '0'  when (neo430_fpga_i2c_scl_o= '0') else 'Z';
    
    infra: entity work.te0712_infra
        generic map(
                USE_NEO430 => True,
                FORCE_RARP => False,
                UID_I2C_ADDR => x"50"
        )
        port map(
            eth_clk_p     => eth_clk_p,
            eth_clk_n     => eth_clk_n,
            eth_rx_p      => eth_rx_p,
            eth_rx_n      => eth_rx_n,
            eth_tx_p      => eth_tx_p,
            eth_tx_n      => eth_tx_n,
            sfp_los       => eth_sfp_los,
            clk_ipb_o     => clk_ipb,
            rst_ipb_o     => rst_ipb,
            clk125_o      => open,
            rst125_o      => open,
            clk200        => open,
            pllclk        => open,
            pllrefclk     => open,
            nuke          => nuke,
            soft_rst      => soft_rst,
            leds          => open,
            mac_addr          => mac_addr,
            ip_addr           => ip_addr,
            ipb_in            => ipb_in,
            ipb_out           => ipb_out,
            fpga_i2c_scl_i    => fpga_i2c_scl,
            fpga_i2c_sda_i    => fpga_i2c_sda,
            fpga_i2c_scl_o    => neo430_fpga_i2c_scl_o,
            fpga_i2c_sda_o    => neo430_fpga_i2c_sda_o,
            uart_rxd_i        => gpio_uart_rd,
            uart_txd_o        => gpio_uart_td,
            gp_o              => open
        );
        
--    mac_addr <= X"020ddba11517";-- Careful here, arbitrary addresses do not always work
--    ip_addr <= X"c0a8c817";

-- ipbus slaves live in the entity below, and can expose top-level ports
-- The ipbus fabric is instantiated within.

    payload: entity work.payload
        port map(
            ipb_clk => clk_ipb,
            ipb_rst => rst_ipb,
            ipb_in => ipb_out,
            ipb_out => ipb_in,
            clk => '0',
            rst => '0',
            nuke => nuke,
            soft_rst => soft_rst,
            userled => open
        );

end rtl;