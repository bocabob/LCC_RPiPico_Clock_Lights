/**
 * Configuration Memory Helper Implementation
 * Auto-generated from OpenLCB configuration
 *
 * Uses chunked memory read/write approach:
 * - OpenLCB datagrams limited to 64 bytes max payload
 * - Large structs read/written in 64-byte chunks
 * - All segments in address space 0xFD (Configuration)
 */

#include "Arduino.h"
#include <LibPrintf.h>
#include "config_mem_helper.h"
#include "openlcb_user_config.h"
#include "src/openlcb/openlcb_application.h"
#include "src/pico/rpi_pico_drivers.h"
#include "src/utilities/mustangpeak_endian_helper.h"
#include <string.h>
#include <stdio.h>

static bool _direct_access = false;

config_mem_t ConfigMemHelper_config_data;
bool ConfigMemHelper_log_access = false;

bool ConfigMemHelper_toggle_log_access(void) {

  ConfigMemHelper_log_access = !ConfigMemHelper_log_access;

  return ConfigMemHelper_log_access;

}

static void _load_defaults_node(openlcb_node_t *openlcb_node, config_mem_t *config, uint16_t *consumer_index, uint16_t *producer_index) {

  const char *name_def = "Southern Piedmont";
  strncpy(config->nodeid.node_name, name_def, sizeof(config->nodeid.node_name)); // Will pad with nulls
  const char *descript_def = "NeoPixel Controller Node";
  strncpy(config->nodeid.node_description, descript_def, sizeof(config->nodeid.node_description)); // Will pad with nulls

}

static void _load_defaults_reset_control(openlcb_node_t *openlcb_node, config_mem_t *config, uint16_t *consumer_index, uint16_t *producer_index) {

  config->reset_control.flag = 238;
  
}

static void _load_defaults_attributes(openlcb_node_t *openlcb_node, config_mem_t *config, uint16_t *consumer_index, uint16_t *producer_index) {

  config->attributes.number_of_strings = 2;
  config->attributes.maximum_luminosity = 100;
  config->attributes.full_luminosity_on = swap_endian64((openlcb_node->id << 16) + *consumer_index); (*consumer_index)++;// EventID for full luminosity on
  config->attributes.minimum_luminosity = 10;
  config->attributes.low_luminosity_on = swap_endian64((openlcb_node->id << 16) + *consumer_index); (*consumer_index)++;  // EventID for low luminosity on
  config->attributes.effect_cycle_frequency = 100;

}

static void _load_defaults_controls(openlcb_node_t *openlcb_node, config_mem_t *config, uint16_t *consumer_index, uint16_t *producer_index) {

  for (int i = 0; i < 6; i++) {
    config->controls.cntrl_group[i].turn_group_on = swap_endian64((openlcb_node->id << 16) + *consumer_index); (*consumer_index)++;  // EventID for group on
    config->controls.cntrl_group[i].turn_group_off = swap_endian64((openlcb_node->id << 16) + *consumer_index); (*consumer_index)++; // EventID for group off
    config->controls.cntrl_group[i].on_hour = 0;    // Default on time hour
    config->controls.cntrl_group[i].on_minute = 0;  // Default on time minute
    config->controls.cntrl_group[i].off_hour = 0;  // Default off time hour
    config->controls.cntrl_group[i].off_minute = 0; // Default off time minute
  }

}

static void _load_defaults_strings(openlcb_node_t *openlcb_node, config_mem_t *config, uint16_t *consumer_index, uint16_t *producer_index) {

  for (int s = 0; s < 4; s++) {
    snprintf(config->strings.string[s].description, sizeof(config->strings.string[s].description), "NeoPixel String %d", s + 1);
    config->strings.string[s].number_of_heads = 5; // Default number of heads
    config->strings.string[s].all_on = swap_endian64((openlcb_node->id << 16) + *consumer_index); (*consumer_index)++;     // EventID for all on
    config->strings.string[s].all_off = swap_endian64((openlcb_node->id << 16) + *consumer_index); (*consumer_index)++;     // EventID for all off
    config->strings.string[s].all_toggle = swap_endian64((openlcb_node->id << 16) + *consumer_index); (*consumer_index)++; // EventID for all toggle
    config->strings.string[s].effects_on = swap_endian64((openlcb_node->id << 16) + *consumer_index); (*consumer_index)++; // EventID for effects on
    config->strings.string[s].effects_off = swap_endian64((openlcb_node->id << 16) + *consumer_index); (*consumer_index)++; // EventID for effects off
    config->strings.string[s].effects_toggle = swap_endian64((openlcb_node->id << 16) + *consumer_index); (*consumer_index)++;// EventID for effects toggle
    config->strings.string[s].dimmer_toggle = swap_endian64((openlcb_node->id << 16) + *consumer_index); (*consumer_index)++;// EventID for dimmer toggle

    for (int n = 0; n < 20; n++) {
      snprintf(config->strings.string[s].neo_pixels[n].description, sizeof(config->strings.string[s].neo_pixels[n].description), "NeoPixel %d", n + 1);
      for (int h = 0; h < 3; h++) {
        config->strings.string[s].neo_pixels[n].head_lead[h].intensity = 50; // Default intensity
        config->strings.string[s].neo_pixels[n].head_lead[h].cycles_on = 5;   // Default cycles on
        config->strings.string[s].neo_pixels[n].head_lead[h].cycles_off = 5;  // Default cycles off
        config->strings.string[s].neo_pixels[n].head_lead[h].starting_cycle = 0;
        config->strings.string[s].neo_pixels[n].head_lead[h].effect = 1;       // Default effect
        config->strings.string[s].neo_pixels[n].head_lead[h].group = 0;        // Default group
      }

    }

  }

}

static void _load_defaults_status(openlcb_node_t *openlcb_node, config_mem_t *config, uint16_t *consumer_index, uint16_t *producer_index) {

  for (int i = 0; i < (2+12+28); i++) {
    config->consumer_status[i] = EVENT_STATUS_UNKNOWN; // Default event state is unknown (0)
  }

  for (int i = 0; i < 1; i++) {
    config->producer_status[i] = EVENT_STATUS_UNKNOWN; // Default event state is unknown (0)
  }

}

uint16_t ConfigMemHelper_config_mem_write(openlcb_node_t *openlcb_node, uint32_t address, uint16_t count, configuration_memory_buffer_t *buffer) {

  // Hook into the Configuration Memory Write to update the data structures in parallel

  // Are we in the internal process of syncing the NMV with the struct?  If so just write to the NVM as we are syncing them.
  if (_direct_access) {  

    delay(10);

    return RPiPicoDrivers_config_mem_write(openlcb_node, address, count, buffer);

  }

  // This is a call from a Configuration Memory Protocol message from an external node or configuration tool so keep the NVM and data structure in sync
  
  if (ConfigMemHelper_log_access) {
    Serial.print("ConfigMemHelper_config_mem_write - Writing Address: ");
    Serial.print(address);
    Serial.print(", count: ");
    Serial.println(count);
  }
  
  // First write the value to the RAM structure
  uint8_t *byte_array = (uint8_t*) &ConfigMemHelper_config_data;
  byte_array += address;
  memcpy(byte_array, buffer, count);

  // Now write to the NVM
  return RPiPicoDrivers_config_mem_write(openlcb_node, address, count, buffer);

}

uint16_t ConfigMemHelper_config_mem_read(openlcb_node_t *openlcb_node, uint32_t address, uint16_t count, configuration_memory_buffer_t *buffer) {

  // Hook into the Configuration Memory Read to update the datastructures in parallel


  // Are we in the internal process of syncing the NMV with the struct?  If so just write to the NVM as we are syncing them.
  if (_direct_access) {

    delay(10);

    return RPiPicoDrivers_config_mem_read(openlcb_node, address, count, buffer);

  }

  // This is a call from a Configuration Memory Protocol message from an external node or configuration tool so the data structures should be in sync with NVM so just read what is in the buffers.

  if (ConfigMemHelper_log_access) {
    Serial.print("ConfigMemHelper_config_mem_read - Reading Address: ");
    Serial.print(address);
    Serial.print(", count: ");
    Serial.println(count);
  }

  uint8_t *byte_array = (uint8_t*) &ConfigMemHelper_config_data;
  byte_array += address;
  memcpy(buffer, byte_array, count);

  // Now read from the NVM
  // return RPiPicoDrivers_config_mem_read(openlcb_node, address, count, buffer);

  return count;

}

bool ConfigMemHelper_reset_and_write_default(openlcb_node_t *openlcb_node) {

  uint16_t consumer_index = 0;
  uint16_t producer_index = 0;

  // Just write this to the NVM don't try to keep the RAM buffer in sync, it is as we want it and just want that image in NVM
  _direct_access = true;

  _load_defaults_node(openlcb_node, &ConfigMemHelper_config_data, &consumer_index, &producer_index);
  _load_defaults_reset_control(openlcb_node, &ConfigMemHelper_config_data, &consumer_index, &producer_index);
  _load_defaults_attributes(openlcb_node, &ConfigMemHelper_config_data, &consumer_index, &producer_index);
  _load_defaults_controls(openlcb_node, &ConfigMemHelper_config_data, &consumer_index, &producer_index);
  _load_defaults_strings(openlcb_node, &ConfigMemHelper_config_data, &consumer_index, &producer_index);
  _load_defaults_status(openlcb_node, &ConfigMemHelper_config_data, &consumer_index, &producer_index);

  if (!ConfigMemHelper_write(openlcb_node, &ConfigMemHelper_config_data)) {

    Serial.println("Failed to write to ConfigMemHelper_write_node");
    _direct_access = false;
    return false;

  }

  _direct_access = false;
  return true;
}

/**
 * Read Config Mem from configuration memory
 *
 * Address: CONFIG_START_ADDR
 * Space: 0xFD (Configuration)
 * Type: config_mem_t
 */
bool ConfigMemHelper_read(openlcb_node_t *openlcb_node, config_mem_t *config) {
  if (!openlcb_node || !config) {
    return false;
  }

  uint32_t address = CONFIG_START_ADDR;  // Starting address from #define
  uint32_t total_size = sizeof(config_mem_t);
  uint32_t bytes_remaining = total_size;
  uint8_t *dest = (uint8_t *)config;

  configuration_memory_buffer_t temp_buffer;

  _direct_access = true;

  // Read in chunks (max 64 bytes per datagram)
  while (bytes_remaining > 0) {
    uint16_t chunk_size = (bytes_remaining > LEN_DATAGRAM_MAX_PAYLOAD)
                            ? LEN_DATAGRAM_MAX_PAYLOAD
                            : bytes_remaining;

    uint16_t bytes_read = ConfigMemHelper_config_mem_read(
      openlcb_node,
      address,
      chunk_size,
      &temp_buffer);

    if (bytes_read != chunk_size) {
      _direct_access = false;
      return false;  // Error or partial read
    }

    // Copy chunk to destination
    memcpy(dest, temp_buffer, chunk_size);

    // Advance pointers for next chunk
    address += chunk_size;
    dest += chunk_size;
    bytes_remaining -= chunk_size;
  }

  _direct_access = false;
  return true;
}

/**
 * Write NODE segment to configuration memory
 *
 * Address: 0x00
 * Space: 0xFD (Configuration)
 * Type: config_mem_t
 *
 * Writes in chunks (max 64 bytes per call) to respect datagram size limits.
 *
 * @param openlcb_node Pointer to OpenLCB node
 * @param config Pointer to nodeid_t struct to write
 * @return true on success, false on error
 */
bool ConfigMemHelper_write(openlcb_node_t *openlcb_node, config_mem_t *config) {
  if (!openlcb_node || !config) {
    return false;
  }

  uint32_t address = NODE_ADDR;  // Starting address from #define
  uint32_t total_size = sizeof(config_mem_t);
  uint32_t bytes_remaining = total_size;
  uint8_t *src = (uint8_t *)config;

  configuration_memory_buffer_t temp_buffer;

  _direct_access = true;

  // Write in chunks (max 64 bytes per datagram)
  while (bytes_remaining > 0) {
    uint16_t chunk_size = (bytes_remaining > LEN_DATAGRAM_MAX_PAYLOAD)
                            ? LEN_DATAGRAM_MAX_PAYLOAD
                            : bytes_remaining;

    // Copy chunk to buffer
    memcpy(temp_buffer, src, chunk_size);

    uint16_t bytes_written = ConfigMemHelper_config_mem_write(
      openlcb_node,
      address,
      chunk_size,
      &temp_buffer);

    if (bytes_written != chunk_size) {

      _direct_access = false;
      return false;  // Error or partial write

    }

    // Advance pointers for next chunk
    address += chunk_size;
    src += chunk_size;
    bytes_remaining -= chunk_size;
  }

  _direct_access = false;
  return true;
}

void ConfigMemHelper_reset_config_mem(void) {

  configuration_memory_buffer_t buffer;

  _direct_access = true;

  memset(&buffer, 0xFF, sizeof(buffer));
  uint16_t address = 0;
  for (unsigned int i = 0; i < (CONFIG_MEM_SIZE / sizeof(buffer)); i++) {

    ConfigMemHelper_config_mem_write(NULL, address, sizeof(buffer), &buffer);
    address = address + sizeof(buffer);
  }

  _direct_access = false;

}

void ConfigMemHelper_clear_config_mem(void) {

  configuration_memory_buffer_t buffer;

  _direct_access = true;

  memset(&buffer, 0x00, sizeof(buffer));
  uint16_t address = 0;
  for (unsigned int i = 0; i < (CONFIG_MEM_SIZE / sizeof(buffer)); i++) {

    ConfigMemHelper_config_mem_write(NULL, address, sizeof(buffer), &buffer);
    address = address + sizeof(buffer);
  }

  _direct_access = false;
}

bool ConfigMemHelper_is_config_mem_reset(void) {

  configuration_memory_buffer_t buffer;
  
  _direct_access = true;

  ConfigMemHelper_config_mem_read(NULL, 0, 1, &buffer);

  _direct_access = false;

  return (buffer[0] == 0xFF);
}
