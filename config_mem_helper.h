#ifndef CONFIG_MEM_HELPER_H
#define CONFIG_MEM_HELPER_H

#include <stdint.h>
#include <stdbool.h>
#include "src/openlcb/openlcb_types.h"

/**
 * Configuration Memory Helper Functions
 * Provides utilities for reading/writing configuration memory
 */

// ===== Configuration Memory Structure =====
// Auto-generated from CDI XML via C Struct Generation Panel

// Auto-generated from CDI XML
// OpenLCB Configuration Memory Structure

#include <stdint.h>

#pragma pack(push, 1)  // Byte-aligned structures

// Memory space 253, origin 0x00
#define CONFIG_START_ADDR 0x00
// Memory space 253, origin 0x00
#define NODE_ADDR 0x00
// Memory space 253, origin 0x7D (125)
#define RESET_CONTROL_ADDR 0x7D
// Memory space 253, origin 0x7E (126)
#define ATTRIBUTES_ADDR 0x7E
// Memory space 253, origin 0x92 (146)
#define CONTROLS_ADDR 0x92
// Memory space 253, origin 0x10A (266)
#define STRINGS_ADDR 0x10A

typedef struct{
  // Memory space 253, origin NODE_ADDR
  struct {           
    char node_name[63];
    char node_description[64];
  } nodeid;
   // Memory space 253, origin RESET_CONTROL_ADDR
  struct {                             
    uint8_t flag;
  } reset_control;
  // Memory space 253, origin ATTRIBUTES_ADDR
  struct {                      
    uint8_t number_of_strings; // Number of NeoPixel strings
    uint8_t maximum_luminosity; // Full luminesence factor
    event_id_t full_luminosity_on;
    uint8_t minimum_luminosity; // Dimmed luminesence factor
    event_id_t low_luminosity_on;
    uint8_t effect_cycle_frequency; // Milliseconds between effect processing
  } attributes;
  // Memory space 253, origin CONTROLS_ADDR
  struct {
    struct {  // Group (replicated 6 times)
        event_id_t turn_group_on;
        event_id_t turn_group_off;
        uint8_t on_hour; // Hour of the day that the group turns on
        uint8_t on_minute; // Minute of the hour that the group turns on
        uint8_t off_hour; // Hour of the day that the group turns off
        uint8_t off_minute; // Minute of the hour that the group turns off
    } cntrl_group[6];
  } controls;
  // Memory space 253, origin STRINGS_ADDR
  struct {
    struct {  // String (replicated 4 times)
        char description[21];
        uint8_t number_of_heads; // Number of heads in the Neo-Pixel string
        event_id_t all_on;
        event_id_t all_off;
        event_id_t all_toggle;
        event_id_t effects_on;
        event_id_t effects_off;
        event_id_t effects_toggle;
        event_id_t dimmer_toggle;
        struct {  // Neo-Pixels (replicated 20 times)
            char description[21];
            struct {  // Head Lead (replicated 3 times)
                uint8_t intensity; // Brightness of the LED
                uint8_t cycles_on; // Number of effect cycles for the LED to be on
                uint8_t cycles_off; // Number of effect cycles for the LED to be off
                uint8_t starting_cycle;
                uint8_t effect;
                uint8_t group;
            } head_lead[3];
        } neo_pixels[20];
    } string[4];
  } strings;
    // modify as desired
  event_status_enum consumer_status[2+12+28]; // Array to hold the state of each event (on/off/unknown) for the 42 events defined in the configuration
  event_status_enum producer_status[1]; // Array to hold the state of each event (on/off/unknown) for the events defined in the configuration

} config_mem_t;

#pragma pack(pop)


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// Global configuration structure instance
extern config_mem_t ConfigMemHelper_config_data;

// ===== Configuration Memory Access Functions =====

// Reads the current NVM into the passed structure, the two are in synce when it returns.
extern bool ConfigMemHelper_read(openlcb_node_t *openlcb_node, config_mem_t *config);
// Writes the current passed structure in the NVM, the two are in synce when it returns.
extern bool ConfigMemHelper_write(openlcb_node_t *openlcb_node, config_mem_t *config);
// Loads the default values for the NMV into the structure and writes them to the NVM, the two are in synce when it returns.
extern bool ConfigMemHelper_reset_and_write_default(openlcb_node_t *openlcb_node);

// Sets all Configuration Memory to the default 0xFF that a newly programmed device would have
extern void ConfigMemHelper_reset_config_mem(void);
// Sets all Configuration Memory to 0x00 which is a valid "cleared memory"
extern void ConfigMemHelper_clear_config_mem(void);
// Tests if the first byte of the Configuration memory is 0xFF, if so then the memory not initialized (cleared 0x00 is initialized as byte 0 is the User Name string which initialized is a null string)
extern bool ConfigMemHelper_is_config_mem_reset(void);

// Hooks for the OpenLcbLib that allows snooping on Config Mem writes before passing them on to the Pico Drivers to write to NVM
extern uint16_t ConfigMemHelper_config_mem_write(openlcb_node_t *openlcb_node, uint32_t address, uint16_t count, configuration_memory_buffer_t *buffer);
// Hooks for the OpenLcbLib that allows snooping on Config Mem reads before passing them on to the Pico Drivers to read/ite to NVM
extern uint16_t ConfigMemHelper_config_mem_read(openlcb_node_t *openlcb_node, uint32_t address, uint16_t count, configuration_memory_buffer_t *buffer);

extern bool ConfigMemHelper_toggle_log_access(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CONFIG_MEM_HELPER_H
