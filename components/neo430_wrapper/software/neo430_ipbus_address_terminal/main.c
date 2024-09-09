// Hacked from "Wishbone Explorer" , part of the NEO430 project by S Nolting ( see below )
// David Cussans, Jan 2020

// #################################################################################################
// #  < Wishbone bus explorer >                                                                    #
// # ********************************************************************************************* #
// # Manual access to the registers of modules, which are connected to Wishbone bus. This is also  #
// # a neat example to illustrate the construction of a console-like user interface.               #
// # ********************************************************************************************* #
// # This file is part of the NEO430 Processor project: https://github.com/stnolting/neo430        #
// # Copyright by Stephan Nolting: stnolting@gmail.com                                             #
// #                                                                                               #
// # This source file may be used and distributed without restriction provided that this copyright #
// # statement is not removed from the file and that any derivative work contains the original     #
// # copyright notice and the associated disclaimer.                                               #
// #                                                                                               #
// # This source file is free software; you can redistribute it and/or modify it under the terms   #
// # of the GNU Lesser General Public License as published by the Free Software Foundation,        #
// # either version 3 of the License, or (at your option) any later version.                       #
// #                                                                                               #
// # This source is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;      #
// # without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     #
// # See the GNU Lesser General Public License for more details.                                   #
// #                                                                                               #
// # You should have received a copy of the GNU Lesser General Public License along with this      #
// # source; if not, download it from https://www.gnu.org/licenses/lgpl-3.0.en.html                #
// # ********************************************************************************************* #
// #  Stephan Nolting, Hannover, Germany                                               06.10.2017  #
// #################################################################################################


// Libraries
#include <stdint.h>
#include <string.h>
#include "neo430_cmd_buffer.h"
#include "neo430.h"
#include "neo430_i2c.h"
#include "neo430_wishbone_mac_ip.h"
#include <stdbool.h>

// Configuration
#define BAUD_RATE 19200

uint64_t uid;
uint32_t ipAddr;
uint16_t gpo; // value to write to general purpose output
bool useRARP;

/* ------------------------------------------------------------
 * Function to read EEPROM and set MAC,IP addresses
 * ------------------------------------------------------------ */
int setMacIP(void){
  
    // configure i2c switch
  // config_i2c_switch(I2C_MUX_CHAN_3);
  
  // set IPBus reset
  neo430_wishbone_writeIPBusReset(true);

  // Then read MAC address
  uid = read_UID();
  uid = ( uid == 0 ) ? 0x020ddba11644 : uid; // if can't read UID, then set to dummy value.
  // and write to control lines
  neo430_wishbone_writeMACAddr(uid);

#if FORCE_RARP == 0
  // then read IP address
  ipAddr = read_Prom();
  // and write to control lines
  neo430_wishbone_writeIPAddr(ipAddr);
#endif

  // if the IP address is set to 255.255.255.255 or 0.0.0.0 then use RARP
  useRARP = ((ipAddr == 0xFFFFFFFF) || (ipAddr == 0) || FORCE_RARP==1 ) ? true : false;
  neo430_wishbone_writeRarpFlag(useRARP);

  //  // then read the value to write to general purpose output (used for endpoint addr in DUNE)
  //gpo = read_PromGPO();
  //neo430_gpio_port_set(gpo);

  // then release IPBus reset line
  neo430_wishbone_writeIPBusReset(false);

  return 0;
}


/* ------------------------------------------------------------
 * INFO Main function
 * ------------------------------------------------------------ */
int main(void) {

  uint16_t length = 0;
  uint16_t selection = 0;
  uint8_t ctrlByte = 0x0;

  // setup UART
  neo430_uart_setup(BAUD_RATE);
  //  USI_CT = (1<<USI_CT_EN);
 
  neo430_uart_br_print( "\n----------------------------------------\n"
                          "- IPBus Address Control Terminal v0.21 -\n"
                          "----------------------------------------\n\n");

  // check if WB unit was synthesized, exit if no WB is available
  if (!(SYS_FEATURES & (1<<SYS_WB32_EN))) {
    neo430_uart_br_print("Error! No WB");
    return 1;
  }


  // set for 32 bit transfer
  //wb_config = 4;

  // set up I2C pre-scale
  setup_i2c();

  // read EEPROM and write to IPBus IP and MAC addresses
  setMacIP();
    
  for (;;) {
    neo430_uart_br_print("\nEnter a command:> ");

    //length = uart_scan(command, MAX_CMD_LENGTH);
    length = neo430_uart_scan(command, MAX_CMD_LENGTH,1);
    neo430_uart_br_print("\n");

    if (!length) // nothing to be done
        continue;

    // decode input
    selection = 0;
    if (!strcmp(command, "help"))
    	selection = 1;
    //    if (!strcmp(command, "config"))
    //	selection = 2;
    if (!strcmp(command, "id"))
    	selection = 3;
#if FORCE_RARP == 0
    if (!strcmp(command, "write"))
    	selection = 4;
    if (!strcmp(command, "read"))
    	selection = 5;
#endif
    //if (!strcmp(command, "writegpo"))
    //  selection = 6;
    //if (!strcmp(command, "readgpo"))
    //  selection = 7;
    if (!strcmp(command, "dump"))
    	selection = 7;
    if (!strcmp(command, "set"))
    	selection = 8;
    if (!strcmp(command, "reset"))
    	selection = 9;

    // execute command
    switch(selection) {

    case 1: // print help menu
        neo430_uart_br_print("Available commands:\n"
                      " help     - show this text\n"
		      //" config    - sets SFP I2C switch channel\n"
                      " id       - read Unique ID\n"
#if FORCE_RARP == 0
                      " write    - write IP addr to PROM\n"
                      " read     - read IP addr from PROM\n"
#endif
		     //" writegpo - write GPO value to PROM\n"
		     //" readgpo  - read GPO value from PROM\n"
		              " dump     - dump EEPROM contents\n"
                      " set      - read from PROM. Set MAC and IP address\n"
                      " reset    - reset CPU\n"
                      );
        break;

    case 2: // Configures I2C switch
        for(;;){
            neo430_uart_br_print("\nWhich Channel to select 0, 1, 2 or 3:> ");
            length = neo430_uart_scan(chan, 2,1);
            neo430_uart_br_print("\n");

            if (!length){// nothing to be done
                continue;
            }
            else if (!strcmp(chan, "0")){
                ctrlByte = I2C_MUX_CHAN_0;
                break;
            }
            else if (!strcmp(chan, "1")){
                ctrlByte = I2C_MUX_CHAN_1;
                break;
            }
            else if(!strcmp(chan, "2")){
                ctrlByte = I2C_MUX_CHAN_2;
                break;
            }
            else if (!strcmp(chan, "3")){
                ctrlByte = I2C_MUX_CHAN_3;
                break;
            }
            else{
                neo430_uart_br_print("\n Please type: 0, 1, 2 or 3\n");
                continue;
            }
        }
        config_i2c_switch(ctrlByte);
        break;

    case 3: // read from Unique ID address
        // config_i2c_switch(I2C_MUX_CHAN_3);
        uid = read_UID();
        print_MAC_address(uid);
        break;

#if FORCE_RARP == 0
    case 4: // write to PROM
        // config_i2c_switch(I2C_MUX_CHAN_3);
        write_Prom();
        break;

    case 5: // read from PROM
        // config_i2c_switch(I2C_MUX_CHAN_3);
        ipAddr = read_Prom();
        print_IP_address(ipAddr);
        break;
#endif

    case 6: // write General Purpose Output value to PROM
        // config_i2c_switch(I2C_MUX_CHAN_3);
        write_PromGPO();

    //case 7: // read GPO value from PROM
         //gpo = read_PromGPO();
         //print_GPO(gpo);

    //case 8: // set MAC , IP address , RARP flag
    //    setMacIP();
    //    break;
    case 7:  // dump entire contents of PROM
        // config_i2c_switch(I2C_MUX_CHAN_3);
        dump_Prom();

    case 9: // restart
        while ((UART_CT & (1<<UART_CT_TX_BUSY)) != 0); // wait for current UART transmission
        neo430_soft_reset();
        break;

    default: // invalid command
        neo430_uart_br_print("bad cmd. 'help' for list.\n");
        break;
    }
  }
     return 0;
}

