/*
 * Pin definitions for LCC RPi Pico Node board v2.8
 *
 * Do not include this file directly — it is selected automatically
 * by BoardSettings.h based on the LCC_BOARD_* define in ProjectConfig.h.
 *
 * Key differences from v2.5/v2.6:
 *   - CAN moved to gp0-gp4 (still SPI0 — same peripheral, different pins)
 *   - I2C1 (storage) moved to gp6 (SDA) / gp7 (SCL)
 *   - I2C0 (servo) on gp16 (SDA) / gp17 (SCL) — valid I2C0 pins on RP2040
 *   - No dedicated NeoPixel pins
 *   - Blue/Gold buttons shared with I/O-2 header (gp21/gp22 — unchanged)
 *
 * v2.8 layout:
 *   NeoPixel outputs : none dedicated — use I/O header pins
 *   I2C0 for servo   : gp16 (SDA) / gp17 (SCL)
 *   I2C1 for storage : gp6 (SDA) / gp7 (SCL)  — Wire1
 *   CAN (SPI0)       : SDO=gp0, CS=gp1, SCK=gp2, SDI=gp3, INT=gp4
 *   Buttons          : Blue=gp21 (I/O-2:Pin4), Gold=gp22 (I/O-2:Pin7)
 */

#ifndef BOARDPINS_NODE_V28_H
#define BOARDPINS_NODE_V28_H

// --------------------------------------------
//  NeoPixel outputs
//  No dedicated pins on v2.8 — assign I/O header pins here if needed.
//  127 = UNUSED_PIN (defined in BoardSettings.h).
// --------------------------------------------
#define NeoPixel_PinA   127  // UNUSED — reassign to an I/O header pin
#define NeoPixel_PinB   127
#define NeoPixel_PinC   127
#define NeoPixel_PinD   127

// --------------------------------------------
//  I2C0 — servo controller (PCA9685)
//  gp16/gp17 are valid I2C0 SDA/SCL pins on RP2040
// --------------------------------------------
#define SERVO_SDA   16
#define SERVO_SCL   17

// --------------------------------------------
//  I2C1 — EEPROM storage (Wire1, gp6/gp7)
// --------------------------------------------
#define I2C_SDA      6
#define I2C_SCL      7
#define STOR_WIRE    Wire1

// --------------------------------------------
//  CAN transceiver — MCP2517/18 via SPI0 (gp0-gp4)
// --------------------------------------------
#define MCP2517_SPI  SPI
#define MCP2517_SDO   0   // MISO (ACAN_RX_PIN)
#define MCP2517_CS    1
#define MCP2517_SCK   2
#define MCP2517_SDI   3   // MOSI (ACAN_TX_PIN)
#define MCP2517_INT   4

// --------------------------------------------
//  Buttons (shared with I/O-2 header)
// --------------------------------------------
#define BLUE_BUTTON_PIN  21   // I/O-2:Pin4
#define GOLD_BUTTON_PIN  22   // I/O-2:Pin7

#endif  // BOARDPINS_NODE_V28_H
