/*
 *  This is the configuration file for the Raspberry Pi Pico node hardware.
 */

#ifndef DEFINES_H
#define DEFINES_H

// ProjectConfig.h is the single source of truth for board selection.
// It is included here so that ALL translation units (.cpp files) get the same
// defines through their BoardSettings.h include chain.
#include "ProjectConfig.h"

#define ARDUINO_COMPATIBLE

// --------------------------------------------
//  Reserved sentinel values for board_configs/ headers.
//  PWR_VCC/PWR_GND/PWR_AGND/PWR_VREF mark connector pins that carry power/
//  ground rather than a GPIO signal. Must be defined before the board
//  dispatch below since board_configs/BoardPins_Node_v30.h uses them.
// --------------------------------------------
#define PWR_VCC     126
#define PWR_GND     125
#define PWR_AGND    124
#define PWR_VREF    123

// --------------------------------------------
//  Board hardware selection
//  Set in ProjectConfig.h — do not define here or in individual .cpp files.
// --------------------------------------------
#if defined(LCC_BOARD_NODE_V25)
  #include "board_configs/BoardPins_Node_v25.h"
#elif defined(LCC_BOARD_NODE_V26)
  #include "board_configs/BoardPins_Node_v26.h"
#elif defined(LCC_BOARD_NODE_V27)
  #include "board_configs/BoardPins_Node_v27.h"
#elif defined(LCC_BOARD_NODE_V28)
  #include "board_configs/BoardPins_Node_v28.h"
#elif defined(LCC_BOARD_NODE_V295)
  #include "board_configs/BoardPins_Node_v295.h"
#elif defined(LCC_BOARD_NODE_V30)
  #include "board_configs/BoardPins_Node_v30.h"
#else
  #error "No board version defined. Set LCC_BOARD_NODE_V25/V26/V27/V28/V295/V30 in ProjectConfig.h"
#endif

// --------------------------------------------
// Select ONE of these for Non-volatile Memory Storage
// --------------------------------------------
// #define USE_INTERNAL_FLASH_STORAGE
#define USE_I2C_STORAGE

// if using external NVM storage, select ONE of these supported types, otherwise use none
#define EXTERNAL_EEPROM     // if uncommented uses external eeprom, using Adafruit library
// #define EXTERNAL_FRAM         // if uncommented uses externalFRAM, using Adafruit library

// if using external NVM, use either Tillaart library or default to Adafruit library
#define USE_TILLAART       // if uncommented then uses Rob Tillaart EEPROM and/or FRAM library

// --------------------------------------------

// -------------------------------------------
// Select ONE of these for Configuration Memory Size
// --------------------------------------------
// #define CONFIG_MEM_SIZE      65536
// 32768 minus 64 bytes reserved for the protected NVM region above config
// memory (node identity block + headroom — see LCC_NODE_STANDARD.md §7.1)
#define CONFIG_MEM_SIZE      32704
// #define CONFIG_MEM_SIZE      16384
//#define CONFIG_MEM_SIZE      8192
//#define CONFIG_MEM_SIZE      4096
//#define CONFIG_MEM_SIZE      2048
//#define CONFIG_MEM_SIZE      1024
//#define CONFIG_MEM_SIZE      512
//#define CONFIG_MEM_SIZE      256
//#define CONFIG_MEM_SIZE      128
// --------------------------------------------

// Define the size of the EEPROM chip or use 4096 if using emulated internal flash storage
// #define I2C_DEVICESIZE      65536  // 24LC512
#define I2C_DEVICESIZE      32768  // 24LC256
// #define I2C_DEVICESIZE      16384  // 24LC128
// #define I2C_DEVICESIZE       8192  // 24LC64
// #define I2C_DEVICESIZE       4096  // 24LC32    // this is the size to use for internal FLASH EEPROM emulation
// #define I2C_DEVICESIZE       2048  // 24LC16
// #define I2C_DEVICESIZE       1024  // 24LC08
// #define I2C_DEVICESIZE        512  // 24LC04
// #define I2C_DEVICESIZE        256  // 24LC02
// #define I2C_DEVICESIZE        128  // 24LC01

/////////////////////////////////////////////////////////////////////////////////////
//  Define a valid (and free) I2C address, 0x60 is the default.
//
// #define I2C_ADDRESS 0x60
#define SERVO_ADDRESS 0x40
// #define EEPROM_ADDRESS 0x50
#define STORAGE_ADDR 0x50  // 0x50 is the default address!

// STOR_WIRE, I2C_SDA, I2C_SCL, NeoPixel_Pin*,
// MCP2517_*, BLUE/GOLD_BUTTON_PIN
// are all defined in the board_configs/BoardPins_*.h file selected above.

/////////////////////////////////////////////////////////////////////////////////////
//  Define the LED blink rates for fast and slow blinking in milliseconds.
//
//  The LED will alternative on/off for these durations.
#define FREQUENCY 100

#define UNUSED_PIN 127

// Define current version of EEPROM configuration
#define EEPROM_VERSION 8

//  Enable debug outputs if required during troubleshooting.
#define NODE_DEBUG true  // uncomment for debug

// NeoPixel defines
// NeoPixel_PinA/B/C/D are defined in the board header above.
#define MAX_STRINGS 4 // also defined in NPlights.cpp and program .ino files and need to update event table
#define MAX_LIGHTS 20 // also defined in NPlights.cpp and program .ino files

#define brightnesss 90
#define MAX_LUMINANCE 100

#endif
