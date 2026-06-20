/*
 * Physical pin topology for LCC RPi Pico Node board v3.0
 *
 * Do not include this file directly — it is selected automatically
 * by BoardSettings.h based on the LCC_BOARD_NODE_V30 define in ProjectConfig.h.
 *
 * This file describes PHYSICAL connector layout only.
 * Functional assignments (display, touch, stepper, NeoPixel, buttons, etc.)
 * depend on which breakout is attached — see NodeConfig.h (this project) and
 * LCC_NODE_STANDARD.md §3 / §6.1 in LCC_RPiPico_Common.
 *
 * Connector layout — 10-pin IDC (IO1, IO2):
 *   Pins 1-4  signal | Pin 5 GND | Pin 6 Vselect (jumper-selectable 3.3V/5V) | Pins 7-10 signal
 *
 * IO3 is a 5-pin analog header:
 *   Pin 1-2 signal | Pin 3 AGND | Pin 4 VREF | Pin 5 signal
 *
 * Changes from v2.9:
 *   - IO2_PIN10 moved from gp26 to gp5 (gp5 was a standalone header pin on
 *     v2.9) — this fully frees gp26 for IO3 use; IO2 and IO3 no longer share
 *     a pin.
 *   - Blue/Gold buttons moved off gp21/gp22 (which are now plain IO2 Pin8/9
 *     with no button function) onto gp5 (shared with IO2 Pin10) and gp28
 *     (shared with IO3 Pin5).
 */

#ifndef BOARDPINS_NODE_V30_H
#define BOARDPINS_NODE_V30_H

// --------------------------------------------
//  IO1 — 10-pin IDC connector
// --------------------------------------------
#define IO1_PIN1    8
#define IO1_PIN2    9
#define IO1_PIN3   10
#define IO1_PIN4   11
#define IO1_PIN5   PWR_GND
#define IO1_PIN6   PWR_VCC   // Vselect — jumper-selectable 3.3V/5V, not fixed VCC
#define IO1_PIN7   12
#define IO1_PIN8   13
#define IO1_PIN9   14
#define IO1_PIN10  15

// --------------------------------------------
//  IO2 — 10-pin IDC connector
//  gp5 = Pin10 (also Blue Button) — moved from gp26 on v2.9
// --------------------------------------------
#define IO2_PIN1   16
#define IO2_PIN2   17
#define IO2_PIN3   18
#define IO2_PIN4   19
#define IO2_PIN5   PWR_GND
#define IO2_PIN6   PWR_VCC   // Vselect — jumper-selectable 3.3V/5V, not fixed VCC
#define IO2_PIN7   20
#define IO2_PIN8   21
#define IO2_PIN9   22
#define IO2_PIN10   5

// --------------------------------------------
//  IO3 — 5-pin analog header
//  Pin 3 = AGND, Pin 4 = VREF (not assignable GPIOs)
//  gp28 = Pin5 (also Gold Button)
// --------------------------------------------
#define IO3_PIN1   26
#define IO3_PIN2   27
#define IO3_PIN3   PWR_AGND
#define IO3_PIN4   PWR_VREF
#define IO3_PIN5   28

// --------------------------------------------
//  Buttons
// --------------------------------------------
#define BLUE_BUTTON_PIN   5   // shared with IO2_PIN10
#define GOLD_BUTTON_PIN  28   // shared with IO3_PIN5

// --------------------------------------------
//  I2C1 — EEPROM storage (Wire1, gp6/gp7)
//  Fixed-function traces — not on any connector.
// --------------------------------------------
#define I2C_SDA      6
#define I2C_SCL      7
#define STOR_WIRE    Wire1

// --------------------------------------------
//  CAN transceiver — MCP2517/18 via SPI0 (gp0-gp4)
//  Fixed-function traces — not on any connector.
// --------------------------------------------
#define MCP2517_SPI  SPI
#define MCP2517_SDO   0   // MISO (ACAN_RX_PIN)
#define MCP2517_CS    1
#define MCP2517_SCK   2
#define MCP2517_SDI   3   // MOSI (ACAN_TX_PIN)
#define MCP2517_INT   4

#endif  // BOARDPINS_NODE_V30_H
