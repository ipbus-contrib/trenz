
// Definitions for functions / constands to read/write OpenCores I2C adaptor
// using NEO420 wishbone.
// I2C routines modified from I2CuHal.py
// David Cussans, Jan 2020

#ifndef NEO430_I2C_H
#define NEO430_I2C_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// Prototypes
void setup_i2c(void);
int16_t read_i2c_address(uint8_t addr , uint8_t n , uint8_t data[]);
bool checkack(uint32_t delayVal);
int16_t write_i2c_address(uint8_t addr , uint8_t nToWrite , uint8_t data[], bool stop);
void dump_wb(void);
uint32_t hex_str_to_uint32(char *buffer);
uint16_t hex_str_to_uint16(char *buffer);
void delay(uint32_t n );
bool config_i2c_switch(uint8_t ctrlByte);
bool wake_ax3_ATSHA204A (); 
int64_t read_UID();
int64_t read_UID();
uint16_t zero_buffer( uint8_t buffer[] , uint16_t elements);

int16_t write_Prom();
uint32_t read_Prom();

int16_t write_PromGPO();
uint16_t read_PromGPO();
void dump_Prom();

int16_t read_i2c_prom( uint8_t startAddress , uint8_t wordsToRead , uint8_t buffer[] );
int16_t write_i2c_prom( uint8_t startAddress , uint8_t wordsToWrite, uint8_t buffer[] );



void uint8_to_decimal_str( uint8_t value , uint8_t *buffer) ;
void print_IP_address( uint32_t ipAddr);
void print_MAC_address( uint64_t macAddr);
void print_GPO( uint16_t gpo);

// #define DEBUG 1
#define DELAYVAL 512

#ifndef MAX_CMD_LENGTH
#define MAX_CMD_LENGTH 16
#endif

#ifndef MAX_N
#define MAX_N 16
#endif

#define ENABLECORE 0x1 << 7
#define STARTCMD 0x1 << 7
#define STOPCMD  0x1 << 6
#define READCMD  0x1 << 5
#define WRITECMD 0x1 << 4
#define ACK      0x1 << 3
#define INTACK   0x1

#define RECVDACK 0x1 << 7
#define BUSY     0x1 << 6
#define ARBLOST  0x1 << 5
#define INPROGRESS  0x1 << 1
#define INTERRUPT 0x1

// define ratio of NEO430 clock to SCL speed.
#define I2C_PRESCALE 0x0400

// Multiply addresses by 4 to go from byte addresses (Wishbone) to Word addresses (IPBus)
#define ADDR_PRESCALE_LOW 0x0
#define ADDR_PRESCALE_HIGH 0x4
#define ADDR_CTRL 0x8
#define ADDR_DATA 0xC
#define ADDR_CMD_STAT 0x10

//#define ADDR_PRESCALE_LOW 0x0
//#define ADDR_PRESCALE_HIGH 0x1
//#define ADDR_CTRL 0x2
//#define ADDR_DATA 0x3
//#define ADDR_CMD_STAT 0x4

// Address on I2C bus of EEPROM is passed over GPIO into the NEO
// TLU = 0x50 (E24AA025E)
// pc053 = 0x53 (E24AA025E)
// Crypto EEPROM on AX3 = 0x64 (Not yet implemented)

// PROM memory address start...
#define PROMMEMORYADDR 0x00

// Define area for general purpose flags.
#define PROMMEMORY_GPO_ADDR 0x10

// UID location in PROM memory ...
// 0xFA is UID location in E24AA025E
// 0x10 is MAC address location in "CryptoEEPROM" on AX3
#ifndef PROMUIDADDR
#define PROMUIDADDR 0xFA
//#define PROMUIDADDR 0x10
#endif

#ifndef I2C_MUX_CHAN_0
#define I2C_MUX_CHAN_0 0x01
#endif

#ifndef I2C_MUX_CHAN_1
#define I2C_MUX_CHAN_1 0x02
#endif

#ifndef I2C_MUX_CHAN_2
#define I2C_MUX_CHAN_2 0x04
#endif

#ifndef I2C_MUX_CHAN_3
#define I2C_MUX_CHAN_3 0x08
#endif

// Number of address bytes needed to address PROM
//  E24AA025E needs one address byte sent AT24C256 needs two
#ifndef PROMNADDRBYTES
#define PROMNADDRBYTES 0x1
#endif


extern uint8_t buffer[MAX_N];
extern char command[MAX_CMD_LENGTH];

#endif
