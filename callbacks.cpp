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
 * \file dependency_injectors.c
 *
 *
 * @author Jim Kueneman
 * @date 31 Dec 2025
 */

#include "callbacks.h"
#include "config_mem_helper.h"
#include "openlcb_user_config.h"
#include "BoardSettings.h"
#include "src/pico/rpi_pico_drivers.h"
#include "src/openlcb/openlcb_application_broadcast_time.h"
#include "ClockDisplay.h"

#include "Arduino.h"

#include <PicoOTA.h>
#include <LittleFS.h>

#include <LibPrintf.h>

#include "src/openlcb/openlcb_utilities.h"
#include "src/openlcb/openlcb_application.h"
#include "src/openlcb/openlcb_buffer_store.h"
#include "src/drivers/canbus/can_buffer_store.h"

extern void PixelCallback(uint16_t callin);

static int16_t _100ms_ticks = 0;

static File _image_file;
static uint32_t _bytes_written = 0;
static bool _log_messages = false;
static bool _firmware_image_valid = false;
static bool _can_transmission_valid = false;

const char _BOOTLOADER_FILENAME[] = "bootloader.bin";

void Callbacks_initialize(void) {

  // TODO: Initialize any librarys needed in this module
}

bool Callbacks_toggle_log_messages(void) {

  Serial.print("Current State of Logging is: ");
  if (!_log_messages) {
    Serial.println("ON");
    _log_messages = true;
  } else {
    Serial.println("OFF");
    _log_messages = false;
  }

  return _log_messages;
}

void Callbacks_on_consumed_event_identified(openlcb_node_t *openlcb_node, uint16_t index, event_id_t *event_id, event_status_enum status, event_payload_t *payload) {

  // If was a direct hit then index points to the nodeID in the consumers list and event_id points to it.
  // If was a range hit then index will be 0xFFFF and the event_id is the event that was hit.

  if (index == 0xFFFF) {

    // Range hit - no action needed

  } else {

    switch (openlcb_node->consumers.list[index].status) {
      case EVENT_STATUS_UNKNOWN:
      case EVENT_STATUS_SET:
      case EVENT_STATUS_CLEAR:
        break;
    }
    
    ConfigMemHelper_config_data.consumer_status[index] = openlcb_node->consumers.list[index].status;

    PixelCallback(index);
  }
}

void Callbacks_on_consumed_event_pcer(openlcb_node_t *openlcb_node, uint16_t index, event_id_t *event_id, event_payload_t *payload) {

  // If was a direct hit then index points to the nodeID in the consumers list and event_id points to it.
  // If was a range hit then index will be 0xFFFF and the event_id is the event that was hit.

  if (index == 0xFFFF) {

    // Range hit - no action needed

  } else {

    ConfigMemHelper_config_data.consumer_status[index] = openlcb_node->consumers.list[index].status;
    
    PixelCallback(index);
  }
}

void Callbacks_on_broadcast_time_time_changed(broadcast_clock_t *clock) {

  if (clock) {

    drawFastClock(clock->state.time.hour, clock->state.time.minute);
    updateClockStatus((float)clock->state.rate.rate);

    printf("Current time: %02d:%02d ", clock->state.time.hour, clock->state.time.minute);
    Serial.print(", Rate: ");
    Serial.println(clock->state.rate.rate);

    for (int i = 0; i < 6; i++) {
      if (ConfigMemHelper_config_data.controls.cntrl_group[i].on_hour*60+ConfigMemHelper_config_data.controls.cntrl_group[i].on_minute != ConfigMemHelper_config_data.controls.cntrl_group[i].off_hour*60+ConfigMemHelper_config_data.controls.cntrl_group[i].off_minute) {  // Check if the group is configured with times to trigger
        if ((clock->state.time.hour == ConfigMemHelper_config_data.controls.cntrl_group[i].on_hour) && (clock->state.time.minute == ConfigMemHelper_config_data.controls.cntrl_group[i].on_minute)) {
          PixelCallback(2 + 2*i);  // This is just an example of how to trigger an event on a schedule.  In this case it will trigger the turn_group_on event for the group that matches the current time.  The event index is just an example and should be replaced with the actual index of the event in the configuration.
        }
        if ((clock->state.time.hour == ConfigMemHelper_config_data.controls.cntrl_group[i].off_hour) && (clock->state.time.minute == ConfigMemHelper_config_data.controls.cntrl_group[i].off_minute)) {
          PixelCallback(2 + 2*i + 1);  // This is just an example of how to trigger an event on a schedule.  In this case it will trigger the turn_group_off event for the group that matches the current time.  The event index is just an example and should be replaced with the actual index of the event in the configuration.
        }
      }
    }
  }

}

void Callbacks_on_100ms_timer_callback(void) {
  
    if (!_can_transmission_valid) {

      if (_100ms_ticks > 10) {
   
        digitalWrite(LED_BUILTIN, !digitalReadFast(LED_BUILTIN)); // Toggle on LED for a heart beat

        _100ms_ticks = 0;  // Keep it on for 5 ticks (500ms)

      }

    } else {

      digitalWrite(LED_BUILTIN, LOW); // Turn off LED

      _can_transmission_valid = false;

    }

  _100ms_ticks++;
}

void Callbacks_on_can_rx_callback(can_msg_t *can_msg) {

  if (_log_messages) {
    gridconnect_buffer_t gridconnect;

    OpenLcbGridConnect_from_can_msg(&gridconnect, can_msg);

    unsigned int i = 0;
    Serial.print("Rx: ");
    while ((gridconnect[i] != 0x00) && (i < sizeof(gridconnect_buffer_t))) {
      Serial.print((char)gridconnect[i]);
      i++;
    }
    Serial.println();
  }

  _can_transmission_valid = true;
  digitalWrite(LED_BUILTIN, HIGH);
}

void Callbacks_on_can_tx_callback(can_msg_t *can_msg) {

  if (_log_messages) {
    gridconnect_buffer_t gridconnect;

    OpenLcbGridConnect_from_can_msg(&gridconnect, can_msg);

    unsigned int i = 0;
    Serial.print("Tx: ");
    while ((gridconnect[i] != 0x00) && (i < sizeof(gridconnect_buffer_t))) {
      Serial.print((char)gridconnect[i]);
      i++;
    }
    Serial.println();
  }

  _can_transmission_valid = true;
  digitalWrite(LED_BUILTIN, HIGH);
}

void Callbacks_alias_change_callback(uint16_t new_alias, node_id_t node_id) {

  // Print out the allocated Alias
  printf("Alias Allocation: 0x%02X  ", new_alias);
  printf("NodeID: 0x%06llX\n\n", node_id);
}

void Callbacks_operations_request_factory_reset(openlcb_statemachine_info_t *statemachine_info, config_mem_operations_request_info_t *config_mem_operations_request_info) {

  ConfigMemHelper_clear_config_mem();
  ConfigMemHelper_reset_and_write_default(OpenLcbUserConfig_node_id);

  printf("Factory Reset: NodeID = 0x%06llX\n", OpenLcbUtilities_extract_node_id_from_openlcb_payload(statemachine_info->incoming_msg_info.msg_ptr, 0));
}

void Callbacks_write_firemware(openlcb_statemachine_info_t *statemachine_info, config_mem_write_request_info_t *config_mem_write_request_info, write_result_t write_result) {

  bool success = false;

  if (_image_file) {

    if (_image_file.write(config_mem_write_request_info->write_buffer[0], config_mem_write_request_info->bytes) == config_mem_write_request_info->bytes) {

      _bytes_written = _bytes_written + config_mem_write_request_info->bytes;
      success = true;

    } else {

      _firmware_image_valid = false;

    }

  }

  Serial.print("Address: ");
  Serial.println(config_mem_write_request_info->address);

  write_result(statemachine_info, config_mem_write_request_info, success);

}

void Callbacks_freeze(openlcb_statemachine_info_t *statemachine_info, config_mem_operations_request_info_t *config_mem_operations_request_info) {

  // Make sure the address space is asking for the Firmware Space
  if (config_mem_operations_request_info->space_info->address_space == CONFIG_MEM_SPACE_FIRMWARE) {

    Serial.println("Requesting Firmware update");
    _bytes_written = 0;            // Reset
    _firmware_image_valid = true;  // Start optomisic

    // Get the file system ready
    LittleFS.begin();  // Will close in unfreeze

    // Detect if the file exists and delete it if necessary
    Serial.println("Detected presense of old image file");
    if (LittleFS.exists(_BOOTLOADER_FILENAME)) {
      if (LittleFS.remove(_BOOTLOADER_FILENAME)) {
        Serial.println("Existing firmware image removed");
      } else {
        Serial.println("Firmware image does not exist");
      }
    }

    // Open the file in "append" mode that will create it if it does not exist
    Serial.println("Opening/Creating image file");
    _image_file = LittleFS.open(_BOOTLOADER_FILENAME, "a");  // Will close in unfreeze
    if (_image_file) {
      Serial.println("File Successfully Opened");
    } else {
      Serial.println("File Failed to Open");
    }

    // Allows the node to reply that it is in Firmware Updgrade Mode if asked
    statemachine_info->openlcb_node->state.firmware_upgrade_active = true;
    // Per the spec tell the configuration tool  we are ready to receive the data
    while (!OpenLcbApplication_send_initialization_event(statemachine_info->openlcb_node)) {};
  }
}

void Callbacks_unfreeze(openlcb_statemachine_info_t *statemachine_info, config_mem_operations_request_info_t *config_mem_operations_request_info) {

  // Make sure the address space is asking for the Firmware Space
  if (config_mem_operations_request_info->space_info->address_space == CONFIG_MEM_SPACE_FIRMWARE) {

    // Close the file.
    _image_file.close();  // Opened in Freeze

    // Only set this as a valid image to install on reboot if we were successful
    if (_firmware_image_valid) {
      Serial.print("SUCCESSFUL image write");
      // Tell the OTA library this is the new firmware image
      picoOTA.begin();  // Setup for a reflash on reboot
      picoOTA.addFile(_BOOTLOADER_FILENAME);
      picoOTA.commit();
    } else {
      Serial.print("FAILED image write");
    }

    // Close the file system
    LittleFS.end();  // Opened in Freeze

    Serial.println("UnFreeze - Requesting Firmware firmware update complete, reboot");

    // Back to normal PIP replies
    statemachine_info->openlcb_node->state.firmware_upgrade_active = false;

    Serial.println("Rebooting in 2 seconds...\n");
    delay(2000);
    rp2040.reboot();
  }
}

bool Callbacks_on_login_complete(openlcb_node_t *openlcb_node) {

  // This will keep being called until it return true so if the Tx buffer is full it will just keep trying on each main loop until it is sent
  
  OpenLcbApplicationBroadcastTime_start(BROADCAST_TIME_ID_DEFAULT_FAST_CLOCK);

  return OpenLcbApplicationBroadcastTime_send_query(OpenLcbUserConfig_node_id, BROADCAST_TIME_ID_DEFAULT_FAST_CLOCK);
  
}
