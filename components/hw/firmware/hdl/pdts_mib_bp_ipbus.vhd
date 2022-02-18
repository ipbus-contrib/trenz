library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.ipbus.all;
use work.ipbus_reg_types.all;
use work.ipbus_decode_top_mib_gbe_bp.all;

entity mib_bp_ipbus is
    port(
        ipb_clk         : in std_logic;
        rst_125         : in std_logic;
        eth_bp_tx_p     : out std_logic;
        eth_bp_tx_n     : out std_logic;
        eth_bp_rx_p     : in std_logic;
        eth_bp_rx_n     : in std_logic;
        clk125          : in std_logic;
        clk125_fr       : in std_logic;
        rst_eth         : in std_logic;
        rst_ipb_ctrl    : in std_logic;
        clk_indep       : in std_logic;
        pma_reset               : in std_logic;
        gt0_pll0outclk_in       : in std_logic;
        gt0_pll0outrefclk_in    : in std_logic;
        gt0_pll1outclk_in       : in std_logic;
        gt0_pll1outrefclk_in    : in std_logic;
        gt0_pll0lock_in         : in std_logic;
        gt0_pll0refclklost_in   : in std_logic;
        user_clk                : in std_logic;
        gtrefclk_out            : in std_logic;
        mmcm_locked             : in std_logic;
        bp_eth_locked           : out std_logic
    );

end entity mib_bp_ipbus;

architecture rtl of mib_bp_ipbus is
    
    -- IPBus controller bus (before fabric select)
    signal ipb_ctrl_in    : ipb_rbus;
    signal ipb_ctrl_out   : ipb_wbus;
    
    -- IPBus slave buses (after fabric select)
    signal ipbw: ipb_wbus_array(N_SLAVES - 1 downto 0);
    signal ipbr: ipb_rbus_array(N_SLAVES - 1 downto 0);
    
    -- Axi stream interface from MAC.
    signal mac_tx_data, mac_rx_data: std_logic_vector(7 downto 0);
    signal mac_tx_valid, mac_tx_last, mac_tx_error, mac_tx_ready, mac_rx_valid, mac_rx_last, mac_rx_error: std_logic;
    
    attribute mark_debug: string;
    attribute mark_debug of mac_tx_data, mac_tx_valid, mac_tx_last, mac_tx_error, mac_rx_data, mac_rx_valid, mac_rx_last, mac_rx_error: signal is "True";
    attribute mark_debug of rst_125, bp_eth_locked, rst_eth, pma_reset, mmcm_locked, rst_ipb_ctrl: signal is "True";
    
begin

-- Ethernet MAC core and PHY interface
    bp_eth: entity work.bp_eth_7s_1000basex_gtp
        port map(
            gt_txp      => eth_bp_tx_p,
            gt_txn      => eth_bp_tx_n ,
            gt_rxp      => eth_bp_rx_p,
            gt_rxn      => eth_bp_rx_n,
            tx_data     => mac_tx_data,
            tx_valid    => mac_tx_valid,
            tx_last     => mac_tx_last,
            tx_error    => mac_tx_error,
            tx_ready    => mac_tx_ready,
            rx_data     => mac_rx_data,
            rx_valid    => mac_rx_valid,
            rx_last     => mac_rx_last,
            rx_error    => mac_rx_error,
            clk125          => clk125,
            clk125_fr       => clk125_fr,
            rst_eth         => rst_eth,
            clk_indep       => clk_indep,
            pma_reset               => pma_reset,
            gt0_pll0outclk_in       => gt0_pll0outclk_in,
            gt0_pll0outrefclk_in    => gt0_pll0outrefclk_in,
            gt0_pll1outclk_in       => gt0_pll1outclk_in,
            gt0_pll1outrefclk_in    => gt0_pll1outrefclk_in,
            gt0_pll0lock_in         => gt0_pll0lock_in,
            gt0_pll0refclklost_in   => gt0_pll0refclklost_in,
            user_clk                => user_clk,
            gtrefclk                => gtrefclk_out,
            mmcm_locked             => mmcm_locked, 
            locked                  => bp_eth_locked
        );

-- ipbus control logic
    bp_ipbus: entity work.ipbus_ctrl
        port map(
            mac_clk      => clk125,
            rst_macclk   => rst_125,        --active high
            ipb_clk      => ipb_clk,
            rst_ipb      => rst_ipb_ctrl,   --active high
            mac_rx_data  => mac_rx_data,
            mac_rx_valid => mac_rx_valid,
            mac_rx_last  => mac_rx_last,
            mac_rx_error => mac_rx_error,
            mac_tx_data  => mac_tx_data,
            mac_tx_valid => mac_tx_valid,
            mac_tx_last  => mac_tx_last,
            mac_tx_error => mac_tx_error,
            mac_tx_ready => mac_tx_ready,
            ipb_out      => ipb_ctrl_out,
            ipb_in       => ipb_ctrl_in,
            RARP_select  => '0',                -- Using static IP
            mac_addr     => X"020ddba1164f",    -- 02:0d:db:a1:16:4f
            ip_addr      => X"c0a879c7",        -- 192.168.121.199
            pkt          => open
        );
        
-- ipbus address decode
    bp_ipb_fabric: entity work.ipbus_fabric_sel
        generic map(
            NSLV => N_SLAVES,
            SEL_WIDTH => IPBUS_SEL_WIDTH
        )
        port map(
            ipb_in => ipb_ctrl_out,
            ipb_out => ipb_ctrl_in,
            sel => ipbus_sel_top_mib_gbe_bp(ipb_ctrl_out.ipb_addr),
            ipb_to_slaves => ipbw,
            ipb_from_slaves => ipbr
        );

-- Config info
    bp_ipd_config: entity work.ipbus_roreg_v
        generic map(
            N_REG => 1,
            DATA(31 downto 24)  => X"00",
            DATA(23 downto 16)  => X"06",
            DATA(15 downto 8)   => X"06",
            DATA(7 downto 0)    => X"05"
        )
        port map(
            ipb_in => ipbw(N_SLV_CONFIG),
            ipb_out => ipbr(N_SLV_CONFIG)
        );
end architecture rtl;
