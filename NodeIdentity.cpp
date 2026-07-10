/*
 * NodeIdentity.cpp — protected NVM node identity block implementation.
 * See NodeIdentity.h and LCC_NODE_STANDARD.md §7.1 for the design.
 */

#include "Arduino.h"
#include <string.h>
#include "NodeIdentity.h"
#include "src/pico/rpi_pico_drivers.h"

static bool _pending = false;
static uint64_t _pending_id = 0;

// Simple XOR-based checksum (per the original design) — not cryptographic,
// just enough to detect a torn/blank/corrupted write.
static uint16_t _checksum(const uint8_t *data, size_t len) {
  uint16_t crc = 0;
  for (size_t i = 0; i < len; i++) {
    if (i % 2 == 0) {
      crc ^= (uint16_t)data[i];
    } else {
      crc ^= (uint16_t)data[i] << 8;
    }
  }
  return crc;
}

uint64_t NodeIdentity_read(void) {

  node_identity_t block;
  uint16_t n = RPiPicoDrivers_nvm_raw_read(NODE_IDENTITY_ADDR, (uint8_t *)&block, sizeof(block));

  if (n != sizeof(block)) {
    return 0;
  }

  if (block.magic != NODE_IDENTITY_MAGIC) {
    return 0;
  }

  uint16_t expected = _checksum((uint8_t *)&block, sizeof(block) - sizeof(block.crc));
  if (block.crc != expected) {
    return 0;
  }

  uint64_t id = 0;
  for (int i = 0; i < 6; i++) {
    id = (id << 8) | block.node_id[i];
  }

  return id;
}

bool NodeIdentity_write(uint64_t node_id) {

  node_identity_t block;
  block.magic = NODE_IDENTITY_MAGIC;
  for (int i = 0; i < 6; i++) {
    block.node_id[i] = (uint8_t)(node_id >> ((5 - i) * 8));
  }
  block.crc = _checksum((uint8_t *)&block, sizeof(block) - sizeof(block.crc));

  uint16_t n = RPiPicoDrivers_nvm_raw_write(NODE_IDENTITY_ADDR, (uint8_t *)&block, sizeof(block));
  if (n != sizeof(block)) {
    return false;
  }

  // EEPROM chips need a few ms after the write transaction to internally
  // commit the page — without this, an immediate reboot (as the 'Y' serial
  // command does) can read back stale/blank data on the very next boot.
  delay(20);

  // Verify by reading back rather than trusting the write call alone.
  return NodeIdentity_read() == node_id;
}

void NodeIdentity_begin_provision(uint64_t node_id) {
  _pending = true;
  _pending_id = node_id;
}

bool NodeIdentity_provision_pending(void) {
  return _pending;
}

uint64_t NodeIdentity_pending_id(void) {
  return _pending_id;
}

bool NodeIdentity_confirm_provision(void) {
  if (!_pending) {
    return false;
  }
  bool ok = NodeIdentity_write(_pending_id);
  _pending = false;
  return ok;
}

void NodeIdentity_cancel_provision(void) {
  _pending = false;
}
