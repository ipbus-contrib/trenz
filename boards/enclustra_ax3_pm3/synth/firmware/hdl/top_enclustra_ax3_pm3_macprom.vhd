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


-- Top-level design for ipbus demo
--
-- This version is for Enclustra AX3 module, using the RGMII PHY on the PM3 baseboard
--
-- IP and MAC addresses read from PROM
--
-- Dave Newbold, 4/10/16
--
-- Modified for MAC address from PROM: David Cussans 8/Jan/21
--
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

use work.ipbus.ALL;

entity top is port(
		osc_clk: in std_logic;
		leds: out std_logic_vector(3 downto 0); -- status LEDs
		cfg: in std_logic_vector(3 downto 0); -- switches
		rgmii_txd: out std_logic_vector(3 downto 0);
		rgmii_tx_ctl: out std_logic;
		rgmii_txc: out std_logic;
		rgmii_rxd: in std_logic_vector(3 downto 0);
		rgmii_rx_ctl: in std_logic;
		rgmii_rxc: in std_logic;
		phy_rstn: out std_logic;
		uid_scl: inout std_logic; -- I2C lines for PROM containing MAC address
		uid_sda: inout std_logic;
		FTDI_TXD : in   std_ulogic; -- UART send data on serial/USB chip
    	FTDI_RXD : out  std_ulogic -- UART receive data on serial/USB chip

	);

end top;

architecture rtl of top is

	signal clk_ipb, rst_ipb, clk_aux, rst_aux, nuke, soft_rst, phy_rst_e, userled: std_logic;
	signal ipb_out: ipb_wbus;
	signal ipb_in: ipb_rbus;
	signal inf_leds: std_logic_vector(1 downto 0);
	signal neo430_scl_o , neo430_sda_o : std_logic;
    
    signal uid_sda_o , uid_scl_o : std_logic := '1'; -- In a full design these would be connected to payload 
                                                     -- and provide access to the I2C bus from IPBus
    
begin

-- Infrastructure

-- Tri-state I2C lines:
	uid_sda <= '0'  when ((uid_sda_o= '0') or (neo430_sda_o= '0')) else 'Z';
	uid_scl <= '0'  when ((uid_scl_o= '0') or (neo430_scl_o= '0')) else 'Z';


	infra: entity work.enclustra_ax3_pm3_macprom_infra
		port map(
			osc_clk => osc_clk,
			clk_ipb_o => clk_ipb,
			rst_ipb_o => rst_ipb,
			rst125_o => phy_rst_e,
			clk_aux_o => clk_aux,
			rst_aux_o => rst_aux,
			nuke => nuke,
			soft_rst => soft_rst,
			leds => inf_leds,
			rgmii_txd => rgmii_txd,
			rgmii_tx_ctl => rgmii_tx_ctl,
			rgmii_txc => rgmii_txc,
			rgmii_rxd => rgmii_rxd,
			rgmii_rx_ctl => rgmii_rx_ctl,
			rgmii_rxc => rgmii_rxc,
			uart_txd_o => FTDI_RXD, -- from NEO UART to host
			uart_rxd_i => FTDI_TXD, -- from host to NEO UART
			uid_scl_i => uid_scl,
			uid_sda_i => uid_sda,
			uid_scl_o => neo430_scl_o,
			uid_sda_o => neo430_sda_o,
			ipb_in => ipb_in,
			ipb_out => ipb_out
		);
		
	leds <= not ('0' & userled & inf_leds);
	phy_rstn <= not phy_rst_e;

-- ipbus slaves live in the entity below, and can expose top-level ports
-- The ipbus fabric is instantiated within.

	payload: entity work.payload
		port map(
			ipb_clk => clk_ipb,
			ipb_rst => rst_ipb,
			ipb_in => ipb_out,
			ipb_out => ipb_in,
			clk => clk_aux,
			rst => rst_aux,
			nuke => nuke,
			soft_rst => soft_rst,
			userled => userled
		);

end rtl;
