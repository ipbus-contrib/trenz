### IPBus Infrastructure Block for Enclustra reading MAC,IP address from PROM ###

`enclustra_ax3_pm3_macprom_infra ` performs the same IPBus decoding functions as `enclustra_ax3_pm3_infra` , but reads the MAC address from a  E24AA025E or compatible EEPROM attached to I2C bus.

```
entity enclustra_ax3_pm3_macprom_infra is
        generic (
          CLK_AUX_FREQ : real := 40.0 ; -- Default: 40 MHz clock - LHC
          UID_I2C_ADDR : std_logic_vector(7 downto 0) := x"53" -- Address on I2C bus of E24AA025E
                );
        port(
                osc_clk: in std_logic; -- 50MHz board crystal clock
                clk_ipb_o: out std_logic; -- IPbus clock
                rst_ipb_o: out std_logic;
                clk125_o: out std_logic;
                rst125_o: out std_logic;
                clk_aux_o: out std_logic; -- 50MHz clock
                rst_aux_o: out std_logic;
                nuke: in std_logic; -- The signal of doom
                soft_rst: in std_logic; -- The signal of lesser doom
                leds: out std_logic_vector(1 downto 0); -- status LEDs
                rgmii_txd: out std_logic_vector(3 downto 0);
                rgmii_tx_ctl: out std_logic;
                rgmii_txc: out std_logic;
                rgmii_rxd: in std_logic_vector(3 downto 0);
                rgmii_rx_ctl: in std_logic;
                rgmii_rxc: in std_logic;
                uart_txd_o : out std_logic; -- UART connection between soft core and serial terminal
                uart_rxd_i : in std_logic;
                uid_scl_o: out std_logic; -- I2C bus connected to EEPROM storing MAC address and (if desired IP address)
                uid_sda_o: out std_logic;
                uid_scl_i: in std_logic;
                uid_sda_i: in std_logic;
                gp_o: out std_logic_vector(11 downto 0); -- General purpose output from soft-core CPU
                ipb_in: in ipb_rbus; -- ipbus
                ipb_out: out ipb_wbus
        );
```

The `uid` I2C bus must be "or-ed" with any other logic that uses the I2C bus. For example:

```
        uid_sda <= '0'  when ((uid_sda_o= '0') or (neo430_sda_o= '0')) else 'Z';
        uid_scl <= '0'  when ((uid_scl_o= '0') or (neo430_scl_o= '0')) else 'Z';

```

See (top_enclustra_ax3_pm3_macprom.vhd) as an example of how this can be done.
