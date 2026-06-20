/*
 * NodeIdentity.h — protected NVM node identity block.
 *
 * Stores the node's 48-bit LCC node ID in a small block above
 * CONFIG_MEM_SIZE, out of reach of the config-memory wipe commands ('c'/'r')
 * and CDI/EEPROM_VERSION resets. See LCC_NODE_STANDARD.md §7.1 in
 * LCC_RPiPico_Common for the full design.
 */

#ifndef NODE_IDENTITY_H
#define NODE_IDENTITY_H

#include <stdint.h>
#include <stdbool.h>
#include "BoardSettings.h"

#define NODE_IDENTITY_MAGIC   0xDEADBEEFUL
#define NODE_IDENTITY_ADDR    CONFIG_MEM_SIZE   // first address of the protected region, above config memory

#pragma pack(push, 1)
typedef struct {
  uint32_t magic;       // NODE_IDENTITY_MAGIC when provisioned
  uint8_t  node_id[6];  // 6-byte big-endian node ID
  uint16_t crc;         // checksum over magic + node_id
} node_identity_t;       // 12 bytes — offset +0 of the 64-byte protected region
#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

// Returns the provisioned 48-bit node ID, or 0 if the identity block is not
// provisioned (blank NVM) or fails its CRC check.
uint64_t NodeIdentity_read(void);

// Writes a new identity block for the given 48-bit node ID. Returns true on success.
bool NodeIdentity_write(uint64_t node_id);

// Two-step 'N' / 'Y' serial provisioning flow — see the .ino's loop() serial
// handler for usage.
void NodeIdentity_begin_provision(uint64_t node_id);
bool NodeIdentity_provision_pending(void);
uint64_t NodeIdentity_pending_id(void);
bool NodeIdentity_confirm_provision(void);
void NodeIdentity_cancel_provision(void);

#ifdef __cplusplus
}
#endif

#endif  // NODE_IDENTITY_H
