// Code to read/write OpenCores I2C adaptor using NEO420 wishbone.
// I2C routines modified from I2CuHal.py
// David Cussans, Jan 2020

// Libraries
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "../include/neo430.h"
#include <../include/neo430_i2c.h>

#ifndef DEBUG
#define DEBUG 0
#endif

uint8_t eepromAddress;

bool checkack(uint32_t delayVal) {

#if DEBUG > 1
neo430_uart_br_print("\nChecking ACK\n");
#endif

  bool inprogress = true;
  bool ack = false;
  uint8_t cmd_stat = 0;
  while (inprogress) {
    delay(delayVal);
    cmd_stat = neo430_wishbone32_read8(ADDR_CMD_STAT);
    inprogress = (cmd_stat & INPROGRESS) > 0;
    ack = (cmd_stat & RECVDACK) == 0;

#if DEBUG > 0
    neo430_uart_print_hex_byte( (uint8_t)ack );
#endif

  }
  return ack;
}

/* ------------------------------------------------------------
 * Delay by looping over "no-op"
 * ------------------------------------------------------------ */
void delay(uint32_t delayVal){
  for (uint32_t i=0;i<delayVal;i++){
    asm volatile ("MOV r3,r3");
  }
}


/* ------------------------------------------------------------
 * Zero buffer
 * ------------------------------------------------------------ */
uint16_t zero_buffer (uint8_t buffer[] , uint16_t elements) {

  for (uint16_t i=0;i<elements;i++){
    buffer[i] = 0;
  }

  return elements;
}

/* ------------------------------------------------------------
 * INFO Configure Wishbone adapter
 * ------------------------------------------------------------ */
void setup_i2c(void) {

  uint16_t prescale = I2C_PRESCALE;

  neo430_uart_br_print("Setting up I2C core\n");

  eepromAddress =  neo430_gpio_port_get() & 0xFF ;
  neo430_uart_br_print("I2C address of EEPROM (hex) = ");
  neo430_uart_print_hex_byte( eepromAddress );
  neo430_uart_br_print("\n");
   
// Disable core
  neo430_wishbone32_write8(ADDR_CTRL, 0);

// Setup prescale
  neo430_wishbone32_write8(ADDR_PRESCALE_LOW , (prescale & 0x00ff) );
  neo430_wishbone32_write8(ADDR_PRESCALE_HIGH, (prescale & 0xff00) >> 8);

#if DEBUG > 1
  uint8_t prescaleByte;
  prescaleByte = neo430_wishbone32_read8(ADDR_PRESCALE_LOW);
  neo430_uart_br_print("\nI2C prescale Low, High byte = ");
  neo430_uart_print_hex_byte( prescaleByte );
  neo430_uart_br_print("\n");
  prescaleByte = neo430_wishbone32_read8(ADDR_PRESCALE_HIGH);
  neo430_uart_print_hex_byte( prescaleByte );
  neo430_uart_br_print("\n");
#endif
      
// Enable core
  neo430_wishbone32_write8(ADDR_CTRL, ENABLECORE);

  // Delay for at least 100us before proceeding
  delay(1000);

  neo430_uart_br_print("\nDone.\n");

}


/* ------------------------------------------------------------
 * INFO Read data from I2C
 * ------------------------------------------------------------ */
int16_t read_i2c_address(uint8_t addr , uint8_t n , uint8_t data[]) {

  //static uint8_t data[MAX_N];

  uint8_t val;
  bool ack;

#if DEBUG > 2
  neo430_uart_br_print("\nReading From I2C.\n");
#endif

  addr &= 0x7f;
  addr = addr << 1;
  addr |= 0x1 ; // read bit
  neo430_wishbone32_write8(ADDR_DATA , addr );
  neo430_wishbone32_write8(ADDR_CMD_STAT, STARTCMD | WRITECMD );
  ack = checkack(DELAYVAL);
  if (! ack) {
      neo430_uart_br_print("\nread_i2c_address: No ACK. Send STOP terminate read.\n");
      neo430_wishbone32_write8(ADDR_CMD_STAT, STOPCMD);
      return 0;
      }

  for (uint8_t i=0; i< n ; i++){

      if (i < (n-1)) {
          neo430_wishbone32_write8(ADDR_CMD_STAT, READCMD);
        } else {
          neo430_wishbone32_write8(ADDR_CMD_STAT, READCMD | ACK | STOPCMD); // <--- This tells the slave that it is the last word
        }
      ack = checkack(DELAYVAL);

#if DEBUG > 2
      neo430_uart_br_print("\nread_i2c_address: ACK = ");
      neo430_uart_print_hex_byte( (uint8_t) ack );
      neo430_uart_br_print("\n");
#endif
      
      val = neo430_wishbone32_read8(ADDR_DATA);

#if DEBUG > 0
      neo430_uart_br_print("\nvalue = ");
      neo430_uart_print_hex_byte( val );
      neo430_uart_br_print("\n");
#endif


      data[i] = val & 0xff;
    }

  return (int16_t) n;

}

/* ------------------------------------------------------------
 * INFO Write data to I2C 
 * ------------------------------------------------------------ */
int16_t write_i2c_address(uint8_t addr , uint8_t nToWrite , uint8_t data[], bool stop) {


  int16_t nwritten = -1;
  uint8_t val;
  bool ack;
  addr &= 0x7f;
  addr = addr << 1;

#if DEBUG > 2
  neo430_uart_br_print("\nWriting to I2C.\n");
#endif

  // Set transmit register (write operation, LSB=0)
  neo430_wishbone32_write8(ADDR_DATA , addr );
  //  Set Command Register to 0x90 (write, start)
  neo430_wishbone32_write8(ADDR_CMD_STAT, STARTCMD | WRITECMD );

  ack = checkack(DELAYVAL);

  if (! ack){
    neo430_uart_br_print("\nwrite_i2c_address: No ACK in response to device-ID. Send STOP and terminate\n");
    neo430_wishbone32_write8(ADDR_CMD_STAT, STOPCMD);
    return nwritten;
  }

  nwritten += 1;

  for ( uint8_t i=0;i<nToWrite; i++){
      val = (data[i]& 0xff);
      //Write slave data
      neo430_wishbone32_write8(ADDR_DATA , val );
      //Set Command Register to 0x10 (write)
      neo430_wishbone32_write8(ADDR_CMD_STAT, WRITECMD);
      ack = checkack(DELAYVAL);
      if (!ack){
          neo430_wishbone32_write8(ADDR_CMD_STAT, STOPCMD);
          return nwritten;
        }
      nwritten += 1;
    }

  if (stop) {
#if DEBUG > 2
    neo430_uart_br_print("\nwrite_i2c_address: Writing STOP\n");
#endif
    neo430_wishbone32_write8(ADDR_CMD_STAT, STOPCMD);
  } else {
#if DEBUG > 0
    neo430_uart_br_print("\nwrite_i2c_address: Returning, no STOP\n");
#endif
  }
    return nwritten;
}


/* ------------------------------------------------------------
 * INFO Wake up ATSHA204A crypto EEPROM on AX3
 * ------------------------------------------------------------ */
bool wake_ax3_ATSHA204A (){

  // See Section 6.1.1 of https://ww1.microchip.com/downloads/en/DeviceDoc/ATSHA204A-Data-Sheet-40002025A.pdf
  // first write a string of zeros to SDA
  // 
   // Set transmit register (write operation, LSB=0)
  neo430_wishbone32_write8(ADDR_DATA , 0x00 );
  //  Set Command Register to 0x90 (write, start)
  neo430_wishbone32_write8(ADDR_CMD_STAT, STARTCMD | WRITECMD | STOPCMD );

  // now try to regain synchronization
  // See section 6.5
  // 
  neo430_wishbone32_write8(ADDR_DATA , 0xFF );
  //  Set Command Register to 0x90 (write, start)
  neo430_wishbone32_write8(ADDR_CMD_STAT, STARTCMD | WRITECMD );
  // send an additional start command followed by a stop command
  neo430_wishbone32_write8(ADDR_CMD_STAT, STARTCMD | STOPCMD );

  return true; // TODO - return a status

}


/* ------------------------------------------------------------
 * INFO Configure I2C switch
 * ------------------------------------------------------------ */
bool config_i2c_switch(uint8_t ctrlByte) {

  bool mystop = true;
  uint8_t I2CSWITCH = 0xE0;
  uint8_t bytesToWrite = 1;
  buffer[0] = ctrlByte;

  neo430_uart_br_print("\nEnabling I2C Channel: ");
  neo430_uart_print_hex_byte( ctrlByte );
  neo430_uart_br_print("\n");

  write_i2c_address(I2CSWITCH , bytesToWrite , buffer, mystop);

  zero_buffer(buffer , sizeof(buffer));

  return true; // TODO: return a status, rather than True all the time...
}


/* ---------------------------*
 *  Read bytes from PROM      *
 * ---------------------------*/
int16_t  read_i2c_prom( uint8_t startAddress ,  // Start address in PROM
			uint8_t  bytesToRead,   // Bytes to read from PROM
			uint8_t buffer[]        // Shared buffer to put the data in.
			){

  bool mystop = false;

  buffer[0] = startAddress;
#if PROMNADDRBYTES == 2
  buffer[1] = startAddress;
#endif

#if DEBUG > 2
  neo430_uart_br_print(" read_i2c_prom: Write device ID: ");
#endif

  write_i2c_address( eepromAddress , PROMNADDRBYTES , buffer, mystop );

#if DEBUG > 2
  neo430_uart_br_print("read_i2c_prom: Read EEPROM memory: ");
  zero_buffer(buffer , bytesToRead);
#endif

  read_i2c_address( eepromAddress , bytesToRead , buffer);

#if DEBUG > 2
  neo430_uart_br_print("Data from EEPROM\n");
  for (uint8_t i=0; i< bytesToRead; i++){
    neo430_uart_br_print("\n");
    neo430_uart_print_hex_dword(buffer[i]);    
  }
#endif

  return 0; // replace with a status at some point
}

/* -------------------------------------*
 *  Print 32 bit number as IP address   *
 * -------------------------------------*/
void print_IP_address( uint32_t ipAddr){


  neo430_uart_br_print("\nIP address from PROM = \n");
  neo430_uart_print_hex_dword(ipAddr);
  neo430_uart_br_print("\n");

#if DEBUG > 1
  neo430_uart_br_print("\nIP Address = ");
  for (uint8_t i = 3; i >= 0 && i<4; --i)
  {
    zero_buffer(buffer,4);
    uint8_to_decimal_str( (uint8_t)((ipAddr>>(i*8))&0xFF)  , buffer);
    neo430_uart_br_print( (char *)buffer  );
    neo430_uart_br_print(".");
  }
  neo430_uart_br_print( "\n"  );
#endif

}

/* -------------------------------------*
 *  Print 64 bit number as MAC address  *
 * -------------------------------------*/
void print_MAC_address( uint64_t uid){
  neo430_uart_br_print("\nUID from PROM  = ");
  neo430_uart_print_hex_qword(uid);
  //neo430_uart_print_hex_dword((uid>>32) & 0xFFFFFFFF );
  //neo430_uart_print_hex_dword(uid & 0xFFFFFFFF );
  neo430_uart_br_print("\n");
}

 /* -------------------------------------*
 *  Print 16 bit number as General Purpose Output value   *
 * -------------------------------------*/
void print_GPO( uint16_t gpo){

  neo430_uart_br_print("\nGPO value from PROM = \n");
  neo430_uart_print_hex_word(gpo);
  neo430_uart_br_print("\n");

}
/* -------------------------------------------------*
 *  Read UID from PROM ( e.g. E24AA025E , AT24C256) *
 * -------------------------------------------------*/
int64_t read_UID(){

  
  //  int16_t status;
  uint64_t uid = 0;
  uint8_t b0, b1, b2, b3, b4, b5;

  neo430_uart_br_print("MAC location in I2C PROM = ");
  neo430_uart_print_hex_byte( PROMUIDADDR );
  neo430_uart_br_print("\n");

  neo430_uart_br_print("Number of address bytes = ");
  neo430_uart_print_hex_byte( PROMNADDRBYTES );
  neo430_uart_br_print("\n");

// Reading all 6 bytes at once doesn't work with cheapie AT24C256 
// Nasty work-around
//const uint8_t bytesToRead = 6;
//  read_i2c_prom( PROMUIDADDR , bytesToRead, buffer );

  const uint8_t bytesToRead = 1;
  read_i2c_prom( PROMUIDADDR , bytesToRead, buffer );
  b0 = buffer[0];
  read_i2c_prom( PROMUIDADDR+1 , bytesToRead, buffer );
  b1 = buffer[0];
  read_i2c_prom( PROMUIDADDR+2 , bytesToRead, buffer );
  b2 = buffer[0];
  read_i2c_prom( PROMUIDADDR+3 , bytesToRead, buffer );
  b3 = buffer[0];
  read_i2c_prom( PROMUIDADDR+4 , bytesToRead, buffer );
  b4 = buffer[0];
  read_i2c_prom( PROMUIDADDR+5 , bytesToRead, buffer );
  b5 = buffer[0];

  // Use this when able to read out 6 bytes at a time
  //uid = (uint64_t)buffer[5] + ((uint64_t)buffer[4]<<8) + ((uint64_t)buffer[3]<<16) + ((uint64_t)buffer[2]<<24) + ((uint64_t)buffer[1]<<32) + ((uint64_t)buffer[0]<<40);
  uid = (uint64_t)b5 + ((uint64_t)b4<<8) + ((uint64_t)b3<<16) + ((uint64_t)b2<<24) + ((uint64_t)b1<<32) + ((uint64_t)b0<<40);

  return uid; // Returns bottom 48-bit UID in a 64-bit word

}


/* ---------------------------*
 *  Read 4 bytes from  PROM ( e.g. E24AA025E , AT24C256)   *
 * ---------------------------*/
uint32_t read_Prom() {

  const uint8_t bytesToRead = 4;
  //  int16_t status;
  uint32_t uid ;

  //status =  read_i2c_prom( startAddress , bytesToRead, buffer );
  read_i2c_prom( PROMMEMORYADDR , bytesToRead, buffer );

  uid = (uint32_t)buffer[3] + ((uint32_t)buffer[2]<<8) + ((uint32_t)buffer[1]<<16) + ((uint32_t)buffer[0]<<24);

  return uid; // Returns 32-bit word read from PROM

}


int16_t write_Prom(){

  uint8_t bytesToWrite = 4;
 
  int16_t status = 0;
  bool mystop = true;

  neo430_uart_br_print("Enter hexadecimal data to write to PROM: 0x");
  neo430_uart_scan(command, 9,1); // 8 hex chars for address plus '\0'
  uint32_t data = hex_str_to_uint32(command);

  // Pack data to write into buffer

  // First the address inside the PROM. Some EEPROM need two address bytes
  buffer[0] = PROMMEMORYADDR;
 #if PROMNADDRBYTES == 2
  buffer[1] = PROMMEMORYADDR;
 #endif

  for (uint8_t i=0; i< bytesToWrite; i++){
    buffer[bytesToWrite-i + PROMNADDRBYTES -1 ] = (data >> (i*8)) & 0xFF ;    
  }

  status = write_i2c_address(eepromAddress , (bytesToWrite+PROMNADDRBYTES), buffer, mystop);

  return status;

}

/* ---------------------------*
 *  Read GPO value from PROM   *
 *  Broken for 2 addr byte proms *
 * ---------------------------*/
uint16_t read_PromGPO() {

  uint8_t bytesToRead = 2;
  //  int16_t status;
  uint16_t gpo ;

  //status =  read_i2c_prom( startAddress , bytesToRead, buffer );
  read_i2c_prom( PROMMEMORY_GPO_ADDR , bytesToRead, buffer );

  gpo = ((uint16_t)buffer[1]) + ((uint16_t)buffer[0]<<8);

  return gpo; // Returns 16-bit word read from PROM

}

/* ---------------------------*
 *  Write  GPO value from PROM     *
 *  Broken for 2 addr byte proms *
 * ---------------------------*/
int16_t write_PromGPO(){

  uint8_t bytesToWrite = 2;
 
  int16_t status = 0;
  bool mystop = true;

  neo430_uart_br_print("Enter hexadecimal data to write to PROM: 0x");
  neo430_uart_scan(command, 5,1); // 4 hex chars for address plus '\0'
  uint16_t data = hex_str_to_uint16(command);

  // Pack data to write into buffer
  buffer[0] = PROMMEMORY_GPO_ADDR;
  
  for (uint8_t i=0; i< bytesToWrite; i++){
    buffer[bytesToWrite-i] = (data >> (i*8)) & 0xFF ;    
  }

  status = write_i2c_address(eepromAddress , (bytesToWrite+1), buffer, mystop);

  return status;

}

void dump_Prom(){

  uint8_t memAddress;
  const uint8_t bytesToRead = 1;
  uint8_t byteRead;

  neo430_uart_br_print("Contents of PROM = ");
  
  for(memAddress =0; memAddress<32; memAddress++) {
    read_i2c_prom( memAddress, bytesToRead, buffer );
    byteRead = buffer[0];

    neo430_uart_print_hex_byte( memAddress );
    neo430_uart_br_print(" ");
    neo430_uart_print_hex_byte( byteRead );
    neo430_uart_br_print("\n");
  }
}

/* ------------------------------------------------------------
 * INFO Hex-char-string conversion function
 * PARAM String with hex-chars (zero-terminated)
 * not case-sensitive, non-hex chars are treated as '0'
 * RETURN Conversion result (32-bit)
 * ------------------------------------------------------------ */
uint32_t hex_str_to_uint32(char *buffer) {

  uint16_t length = strlen(buffer);
  uint32_t res = 0, d = 0;
  char c = 0;

  while (length--) {
    c = *buffer++;

    if ((c >= '0') && (c <= '9'))
      d = (uint32_t)(c - '0');
    else if ((c >= 'a') && (c <= 'f'))
      d = (uint32_t)((c - 'a') + 10);
    else if ((c >= 'A') && (c <= 'F'))
      d = (uint32_t)((c - 'A') + 10);
    else
      d = 0;

    res = res + (d << (length*4));
  }

  return res;
}

/* ------------------------------------------------------------
 * INFO Hex-char-string conversion function
 * PARAM String with hex-chars (zero-terminated)
 * not case-sensitive, non-hex chars are treated as '0'
 * RETURN Conversion result (16-bit)
 * ------------------------------------------------------------ */
uint16_t hex_str_to_uint16(char *buffer) {

  uint16_t length = strlen(buffer);
  uint16_t res = 0, d = 0;
  char c = 0;

  while (length--) {
    c = *buffer++;

    if ((c >= '0') && (c <= '9'))
      d = (uint16_t)(c - '0');
    else if ((c >= 'a') && (c <= 'f'))
      d = (uint16_t)((c - 'a') + 10);
    else if ((c >= 'A') && (c <= 'F'))
      d = (uint16_t)((c - 'A') + 10);
    else
      d = 0;

    res = res + (d << (length*4));
  }

  return res;
}

/* -----------------------------------
 * Convert uint8_t into a decimal string
 * Without using divide - we don't want
 * to implement divide unit and we 
 * don't care about speed.
 * ----------------------------------- */
void uint8_to_decimal_str( uint8_t value , uint8_t *buffer) {

  const uint8_t magnitude[3] = {1,10,100};

  uint16_t delta;
  uint16_t trialValue = 0;

  const char ASCII_zero_character = 48;

  buffer[0] = ASCII_zero_character; buffer[1] = ASCII_zero_character; buffer[2] = ASCII_zero_character; buffer[3] = 0;

  //printf("Start, converting %i\n",value);
  //for ( int i =0; i<4; i++){
  //  printf("%i , %i\n",i,buffer[i]);
  //}

  // loop through 100's , 10's and 1's
  for ( int16_t magnitudeIdx =2; magnitudeIdx > -1; magnitudeIdx-- ){

    delta = magnitude[magnitudeIdx];

    // printf("Delta = %i\n",delta);

    // for each magnitude
    for ( uint16_t digit = 0; digit < 10 ; digit ++ ){

      // printf("trialValue = %i\n",trialValue);

      if (( value - ( trialValue + delta )) >= 0) {

	  trialValue += delta;
	  buffer[2-magnitudeIdx] += 1;

	} else {
	  break; // go to the next order of magnitude.
	}
    }
  }

  //for ( int i =0; i<4; i++){
  //  printf("%i , %i\n",i,buffer[i]);
  //}
  return;

}

