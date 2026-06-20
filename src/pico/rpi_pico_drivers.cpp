
/** \copyright
 * Copyright (c) 2025, Jim Kueneman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \rpi_pico_drivers.c
 *
 *
 *
 * @author Jim Kueneman
 * @date 7 Nov 2025
 */


#include "rpi_pico_drivers.h"
#include "rpi_pico_can_drivers.h"

#include "../../BoardSettings.h"

//#define PRINT_DEBUG_RPI_PICO_DRIVERS

#ifdef ARDUINO_COMPATIBLE
// TODO:  include any header files the Raspberry Pi Pico need to compile under Arduino/PlatformIO
  #include "Arduino.h"
  #include "pico/stdlib.h"
  #include "pico/time.h"
  #include <stdio.h>

  #if defined(USE_I2C_STORAGE) && defined(USE_INTERNAL_FLASH_STORAGE)
    #warning "Only USE_I2C_STORAGE OR USE_INTERNAL_FLASH_STORAGE should be defined, not both in rpi_pico_drivers.h"
  #endif

#endif  // ARDUINO_COMPATIBLE

#include "../openlcb/openlcb_config.h"
#include "../openlcb/openlcb_types.h"
#include "../openlcb/openlcb_defines.h"
#include "../utilities/mustangpeak_string_helper.h"

#include "Wire.h"
/*
    Raspberry Pi Pico family              
*/
#if defined(ARDUINO_ARCH_RP2040)
  #if defined(EXTERNAL_EEPROM)
    #pragma message("Using external EEPROM for Pico")
    #if defined(USE_TILLAART)
      #pragma message("Using Tillaart external EEPROM for Pico")
      #include "I2C_eeprom.h"
      I2C_eeprom i2c_eeprom(STORAGE_ADDR, I2C_DEVICESIZE, &STOR_WIRE);
    #else
      #include "Adafruit_EEPROM_I2C.h"
        Adafruit_EEPROM_I2C i2c_eeprom;
    #endif
  #else
    #if defined(EXTERNAL_FRAM)
      #pragma message("Using external FRAM for Pico")
      #include "Wire.h"
      #if defined(USE_TILLAART)
        #pragma message("Using Tillaart external FRAM for Pico")
        #include "FRAM.h"
        FRAM i2c_eeprom(&STOR_WIRE);
      #else
        #include "Adafruit_FRAM_I2C.h"
        Adafruit_FRAM_I2C i2c_eeprom;
      #endif
    #else 
      #pragma message("Using internal EEPROM for Pico")
      #include "EEPROM.h"
    #endif  
  #endif  
#endif // pico

// Create a Timer interrupt or task and call the following
struct repeating_timer timer;
static bool timer_enabled = false;
static bool timer_unhandled_tick = false;

// #ifdef USE_I2C_STORAGE
// I2C_eeprom i2c_eeprom(0x50, I2C_DEVICESIZE);
// #endif  // USE_I2C_STORAGE

void _handle_timer_tick(void) {

  OpenLcbConfig_100ms_timer_tick();

}

bool timer_task_or_interrupt(__unused struct repeating_timer *timer) {

  // a Timer interrupt or task (if using RTOS) and call the following every 100ms or so (not critical)
  if (timer_enabled) {

    _handle_timer_tick();

  } else {

    timer_unhandled_tick = true;
  }

  return true;  // Keep the timer running
}

void RPiPicoDriver_setup(void) {
  // Initialize Raspberry Pi Pico interrupt and any features needed for config memory read/writes

  timer_enabled = add_repeating_timer_ms(-100, timer_task_or_interrupt, NULL, &timer);

  STOR_WIRE.setSDA(I2C_SDA);
  STOR_WIRE.setSCL(I2C_SCL);
  STOR_WIRE.begin();

#ifdef USE_I2C_STORAGE
  #if defined(EXTERNAL_EEPROM)    // Using external EEPROM for Pico
    #if defined(USE_TILLAART)     // Using Tillaart external EEPROM for Pico
      // I2C_eeprom i2c_eeprom(STORAGE_ADDR, I2C_DEVICESIZE, &STOR_WIRE);
      // i2c_eeprom.begin(I2C_EEPROM_WRITE_PIN);
      i2c_eeprom.begin();
    #else
        // Adafruit_EEPROM_I2C i2c_eeprom;
      i2c_eeprom.begin(STORAGE_ADDR, &STOR_WIRE);
    #endif
  #elif defined(EXTERNAL_FRAM)      // Using external FRAM for Pico
    #if defined(USE_TILLAART)     // Using Tillaart external FRAM for Pico
      // FRAM i2c_eeprom(&STOR_WIRE);
      i2c_eeprom.begin(STORAGE_ADDR);
    #else
      // Adafruit_FRAM_I2C i2c_eeprom;
      i2c_eeprom.begin(STORAGE_ADDR, &STOR_WIRE);
    #endif
  #else   // Using internal EEPROM for Pico
    // do not begin, done in function
  #endif  
#endif  // USE_I2C_STORAGE
}

void RPiPicoDrivers_reboot(openlcb_statemachine_info_t *statemachine_info, config_mem_operations_request_info_t *config_mem_operations_request_info) {

  rp2040.reboot();
}

uint16_t RPiPicoDrivers_config_mem_read(openlcb_node_t *openlcb_node, uint32_t address, uint16_t count, configuration_memory_buffer_t *buffer) {

#ifdef PRINT_DEBUG_RPI_PICO_DRIVERS
  Serial.print("RPiPicoDrivers_config_mem_read - Reading Address: 0x");
  Serial.print(address, HEX);
  Serial.print(", Count: 0x");
  Serial.println(count, HEX);
#endif

  if (address > CONFIG_MEM_SIZE - 1) {

    Serial.println("Over ran the Configuration Memory Write.");
    return 0;

  }

  //-----------------------------------------------
  // Don't overrun the buffer
  //-----------------------------------------------
  if ((address + count) >= CONFIG_MEM_SIZE - 1) {

    count = CONFIG_MEM_SIZE - address;
  }
//-----------------------------------------------

//-----------------------------------------------
// Internal FLASH used as EEPROM
//-----------------------------------------------
#if defined(USE_INTERNAL_FLASH_STORAGE) && defined(ARDUINO_COMPATIBLE)

  EEPROM.begin(I2C_DEVICESIZE);

  for (int i = 0; i < count; i++) {

    (*buffer)[i] = EEPROM.read(address + i);
  }

  EEPROM.end();

  return count;

#endif  // defined(USE_INTERNAL_FLASH_STORAGE) && defined(ARDUINO_COMPATIBLE)
//-----------------------------------------------
//-----------------------------------------------
// External NVM accessed via I2C by Rob Tillaart's I2C_EEPROM or FRAM_I2C Library or one's by Adafruit
//-----------------------------------------------
#if defined(USE_I2C_STORAGE) && defined(ARDUINO_COMPATIBLE)
  #if defined(EXTERNAL_EEPROM)    // Using external EEPROM for Pico
    #if defined(USE_TILLAART)     // Using Tillaart external EEPROM for Pico      
      uint16_t bytes_read = i2c_eeprom.readBlock(address, (uint8_t *)buffer, count);
    #else        // Adafruit_EEPROM_I2C i2c_eeprom;  
      bool bytes_read = i2c_eeprom.read(address, count, (uint8_t *)buffer);
    #endif
  #elif defined(EXTERNAL_FRAM)      // Using external FRAM for Pico
    #if defined(USE_TILLAART)     // Using Tillaart external FRAM for Pico  
      uint16_t bytes_read = count;
      i2c_eeprom.read(address, (uint8_t *)buffer, count);
    #else      // Adafruit_FRAM_I2C i2c_eeprom; 
      bool bytes_read = i2c_eeprom.read(address, count, (uint8_t *)buffer);
    #endif
  #endif

   #if defined(USE_TILLAART)
  if (bytes_read != count) {   
    Serial.println("ConfigMemory Read Failed.  Did you define the correct CONFIG_MEM_SIZE in rpi_pico_drivers.h, or correct I2C pins"); 
    return 0;     
    }
  #else
   if (!bytes_read) {   // FALSE
    Serial.println("ConfigMemory Read Failed.  Did you define the correct CONFIG_MEM_SIZE in rpi_pico_drivers.h, or correct I2C pins"); 
    return 0;     
    }
  #endif

  return bytes_read;    
#endif  // defined(USE_I2C_STORAGE) && defined(ARDUINO_COMPATIBLE)

  //-----------------------------------------------
  // // Default response is do nothing
  //-----------------------------------------------
  // Default response is do nothing
  (*buffer)[0] = 0x00;
  return 0;
  //-----------------------------------------------
}

uint16_t RPiPicoDrivers_config_mem_write(openlcb_node_t *openlcb_node, uint32_t address, uint16_t count, configuration_memory_buffer_t *buffer) {

#ifdef PRINT_DEBUG_RPI_PICO_DRIVERS
  Serial.print("RPiPicoDrivers_config_mem_read - Writing Address: 0x");
  Serial.print(address, HEX);
  Serial.print(", Count: 0x");
  Serial.println(count, HEX);
#endif


   if (address > CONFIG_MEM_SIZE - 1) {

    Serial.println("Over ran the Configuration Memory Write.");
    return 0;

   }

  //-----------------------------------------------
  // Don't overrun the buffer
  //-----------------------------------------------
  if (address >= CONFIG_MEM_SIZE - 1) {

    address = CONFIG_MEM_SIZE - 1;
  }
//-----------------------------------------------

//-----------------------------------------------
// Internal FLASH used as EEPROM
//-----------------------------------------------
#if defined(USE_INTERNAL_FLASH_STORAGE) && defined(ARDUINO_COMPATIBLE)

  EEPROM.begin(I2C_DEVICESIZE);

  for (int i = 0; i < count; i++) {

    EEPROM.write(address + i, (*buffer)[i]);
  }

  if (!EEPROM.end()) {

    Serial.println("ConfigMemory Write Failed.  Did you define the correct CONFIG_MEM_SIZE in rpi_pico_drivers.h");

    return 0;
  }

  return count;

#endif  // defined(USE_INTERNAL_FLASH_STORAGE) && defined(ARDUINO_COMPATIBLE)
//-----------------------------------------------

//-----------------------------------------------
// External NVM accessed via I2C by Rob Tillaart's I2C_EEPROM or FRAM_I2C Library or one's by Adafruit
//-----------------------------------------------
#if defined(USE_I2C_STORAGE) && defined(ARDUINO_COMPATIBLE)
  #if defined(EXTERNAL_EEPROM)    // Using external EEPROM for Pico
    #if defined(USE_TILLAART)     // Using Tillaart external EEPROM for Pico
      int bytes_written = i2c_eeprom.writeBlock(address, (uint8_t *)buffer, count);
    #else        // Adafruit_EEPROM_I2C i2c_eeprom;  
      bool bytes_written = i2c_eeprom.write(address, count, (uint8_t *)buffer);
    #endif
  #elif defined(EXTERNAL_FRAM)      // Using external FRAM for Pico
    #if defined(USE_TILLAART)     // Using Tillaart external FRAM for Pico  
      int bytes_written = count;
      i2c_eeprom.write(address, (uint8_t *)buffer, count);
    #else      // Adafruit_FRAM_I2C i2c_eeprom; 
      bool bytes_written = i2c_eeprom.write(address, count, (uint8_t *)buffer);
    #endif
  #endif

  #if defined(USE_TILLAART) && defined(EXTERNAL_EEPROM)
  if (bytes_written != 0) {   // S_OK
    Serial.println("ConfigMemory Write Failed.  Did you define the correct CONFIG_MEM_SIZE in rpi_pico_drivers.h, or correct I2C pins"); 
    return 0;     
    }
  #else
   if (!bytes_written) {   // FALSE
    Serial.println("ConfigMemory Write Failed.  Did you define the correct CONFIG_MEM_SIZE in rpi_pico_drivers.h, or correct I2C pins"); 
    return 0;     
    }
  #endif

  return count;
#endif  // defined(USE_I2C_STORAGE) && defined(ARDUINO_COMPATIBLE)

  //-----------------------------------------------

  //-----------------------------------------------
  // Default response is do nothing
  //-----------------------------------------------
  return 0;
  //-----------------------------------------------
}

//-----------------------------------------------
// Unbounded NVM read/write for the protected identity region above
// CONFIG_MEM_SIZE (see LCC_NODE_STANDARD.md §7.1). Deliberately skips the
// CONFIG_MEM_SIZE bounds check that RPiPicoDrivers_config_mem_read/write
// enforce — that check is what keeps the config-memory wipe commands ('c'/'r')
// from reaching this region, so do NOT add a bounds check here, and do NOT
// use these functions for CDI-driven configuration memory.
//-----------------------------------------------
uint16_t RPiPicoDrivers_nvm_raw_read(uint32_t address, uint8_t *buffer, uint16_t count) {

#if defined(USE_INTERNAL_FLASH_STORAGE) && defined(ARDUINO_COMPATIBLE)

  EEPROM.begin(I2C_DEVICESIZE);

  for (int i = 0; i < count; i++) {
    buffer[i] = EEPROM.read(address + i);
  }

  EEPROM.end();

  return count;

#endif  // defined(USE_INTERNAL_FLASH_STORAGE) && defined(ARDUINO_COMPATIBLE)

#if defined(USE_I2C_STORAGE) && defined(ARDUINO_COMPATIBLE)
  #if defined(EXTERNAL_EEPROM)
    #if defined(USE_TILLAART)
      uint16_t bytes_read = i2c_eeprom.readBlock(address, buffer, count);
    #else
      bool bytes_read = i2c_eeprom.read(address, count, buffer);
    #endif
  #elif defined(EXTERNAL_FRAM)
    #if defined(USE_TILLAART)
      uint16_t bytes_read = count;
      i2c_eeprom.read(address, buffer, count);
    #else
      bool bytes_read = i2c_eeprom.read(address, count, buffer);
    #endif
  #endif

  #if defined(USE_TILLAART)
    if (bytes_read != count) {
      Serial.println("NVM identity region read failed.");
      return 0;
    }
  #else
    if (!bytes_read) {
      Serial.println("NVM identity region read failed.");
      return 0;
    }
  #endif

  return count;
#endif  // defined(USE_I2C_STORAGE) && defined(ARDUINO_COMPATIBLE)

  return 0;
}

uint16_t RPiPicoDrivers_nvm_raw_write(uint32_t address, uint8_t *buffer, uint16_t count) {

#if defined(USE_INTERNAL_FLASH_STORAGE) && defined(ARDUINO_COMPATIBLE)

  EEPROM.begin(I2C_DEVICESIZE);

  for (int i = 0; i < count; i++) {
    EEPROM.write(address + i, buffer[i]);
  }

  if (!EEPROM.end()) {
    Serial.println("NVM identity region write failed.");
    return 0;
  }

  return count;

#endif  // defined(USE_INTERNAL_FLASH_STORAGE) && defined(ARDUINO_COMPATIBLE)

#if defined(USE_I2C_STORAGE) && defined(ARDUINO_COMPATIBLE)
  #if defined(EXTERNAL_EEPROM)
    #if defined(USE_TILLAART)
      int bytes_written = i2c_eeprom.writeBlock(address, buffer, count);
    #else
      bool bytes_written = i2c_eeprom.write(address, count, buffer);
    #endif
  #elif defined(EXTERNAL_FRAM)
    #if defined(USE_TILLAART)
      int bytes_written = count;
      i2c_eeprom.write(address, buffer, count);
    #else
      bool bytes_written = i2c_eeprom.write(address, count, buffer);
    #endif
  #endif

  #if defined(USE_TILLAART) && defined(EXTERNAL_EEPROM)
    if (bytes_written != 0) {
      Serial.println("NVM identity region write failed.");
      return 0;
    }
  #else
    if (!bytes_written) {
      Serial.println("NVM identity region write failed.");
      return 0;
    }
  #endif

  return count;
#endif  // defined(USE_I2C_STORAGE) && defined(ARDUINO_COMPATIBLE)

  return 0;
}

void RPiPicoDrivers_lock_shared_resources(void) {

  // Pause the CAN Rx thread
  RPiPicoCanDriver_pause_can_rx();

  // Pause the 100ms Timer here
  timer_enabled = false;
}

void RPiPicoDrivers_unlock_shared_resources(void) {
  // Resume the CAN Rx thread
  RPiPicoCanDriver_resume_can_rx();

  // Always re-enable the timer -- the new OpenLcbConfig_100ms_timer_tick() only
  // increments a counter in the ISR (no protocol work), so lock_shared_resources
  // disables it only to prevent a partial-count race.  It must be unconditionally
  // restored here; without this the fast lock/unlock cycles in OpenLcbConfig_run()
  // keep timer_enabled false indefinitely, starving the 100ms tick counter and
  // preventing on_100ms_timer callbacks from ever firing.
  timer_enabled = true;

  if (timer_unhandled_tick) {

    timer_unhandled_tick = false;
    _handle_timer_tick();
  }
}
