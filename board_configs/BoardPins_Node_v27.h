/*
 * Pin definitions for LCC RPi Pico Node board v2.7
 *
 * Do not include this file directly — it is selected automatically
 * by BoardSettings.h based on the LCC_BOARD_* define in ProjectConfig.h.
 *
 * Key differences from v2.5/v2.6:
 *   - No dedicated NeoPixel pins; gp0-gp7 are general-purpose I/O headers.
 *     Connect NeoPixel strings to I/O header pins and update NeoPixel_Pin*
 *     defines here if needed.
 *   - I2C0 (servo) on natural I2C0 pins gp4/gp5 (I/O-1:Pin7/Pin8)
 *   - I2C1 (storage), CAN, and buttons unchanged from v2.5
 *
 * v2.7 layout:
 *   NeoPixel outputs : none dedicated — use I/O header pins
 *   I2C0 for servo   : gp4 (SDA) / gp5 (SCL)  — I/O-1:Pin7/Pin8
 *   I2C1 for storage : gp26 (SDA) / gp27 (SCL)  — Wire1
 *   CAN (SPI0)       : SDO=gp16, CS=gp17, SCK=gp18, SDI=gp19, INT=gp20
 *   Buttons          : Blue=gp21, Gold=gp22
 */

#ifndef BOARDPINS_NODE_V27_H
#define BOARDPINS_NODE_V27_H

// --------------------------------------------
//  NeoPixel outputs
//  No dedicated pins on v2.7 — assign I/O header pins here if needed.
//  127 = UNUSED_PIN (defined in BoardSettings.h).
// --------------------------------------------
#define NeoPixel_PinA   2  // UNUSED — reassign to an I/O header pin
#define NeoPixel_PinB   6
#define NeoPixel_PinC   7
#define NeoPixel_PinD   3

// --------------------------------------------
//  I2C0 — servo controller (PCA9685)
//  gp4/gp5 = I/O-1:Pin7/Pin8, natural I2C0 pins on RP2040
// --------------------------------------------
#define SERVO_SDA    4
#define SERVO_SCL    5

// --------------------------------------------
//  I2C1 — EEPROM storage (Wire1) — unchanged from v2.5
// --------------------------------------------
#define I2C_SDA     26
#define I2C_SCL     27
#define STOR_WIRE   Wire1

// --------------------------------------------
//  CAN transceiver — MCP2517/18 via SPI0 — unchanged from v2.5
// --------------------------------------------
#define MCP2517_SPI SPI
#define MCP2517_SDO 16
#define MCP2517_CS  17
#define MCP2517_SCK 18
#define MCP2517_SDI 19
#define MCP2517_INT 20

// --------------------------------------------
//  Buttons — unchanged from v2.5
// --------------------------------------------
#define BLUE_BUTTON_PIN  21
#define GOLD_BUTTON_PIN  22


// --------------------------------------------
//  Display geometry & orientation
//  ER-TFTMC070-4 7" LT7381 (RA8876 clone), 800×480
// --------------------------------------------
#define HRES           800
#define VRES           480
#define ROTATION          0
// --------------------------------------------
//  Touch — XPT2046 resistive, soft SPI
//    CLK / DIN share gp10 / gp11 with the display (driven as GPIO during touch reads).
//    DOUT on dedicated gp13 — separate wire from FFC pin 36 (RA8876 does not tristate SDO).
//    CS on gp12.
// --------------------------------------------
// Touch — XPT2046, hardware SPI1 with switched MISO
//   TOUCH_CS   gp15 → FFC pin 32 (RTP_/CS)
//   TOUCH_MISO gp12 → FFC pin 36 (RTP_DOUT)  ← valid SPI1_RX alternate pin on RP2040
//   SCK/MOSI share gp10/gp11 with RA8876; SPI1 MISO is switched between gp8 (RA8876) and gp12 (XPT2046)
#define TOUCH_CS    15
#define TOUCH_MISO  12

// --------------------------------------------
//  Display — LT7381 (RA8876 clone) via SPI1 on I/O-1
//
//  Naming from the RP2040's perspective:
//    DISPLAY_SDO = RP2040 serial data OUT (SPI1 TX, gp11) → display SDI
//    DISPLAY_SDI = RP2040 serial data IN  (SPI1 RX, gp8)  ← display SDO
//
//  DISPLAY_SDO is passed as the 'mosi' argument to TT_Display / RA8876_RP2040,
//  so it must be gp11 (SPI1 TX).  DISPLAY_SDI is the 'miso' argument → gp8.
// --------------------------------------------
#define DISPLAY_SPI     SPI1
#define DISPLAY_SDI      8   // I/O-1:Pin1 — SPI1 RX  (MISO ← display SDO)
#define DISPLAY_CS       9   // I/O-1:Pin2
#define DISPLAY_SCK     10   // I/O-1:Pin3
#define DISPLAY_SDO     11   // I/O-1:Pin4 — SPI1 TX  (MOSI → display SDI)
#define DISPLAY_RST     14   // I/O-1:Pin9

#endif  // BOARDPINS_NODE_V27_H
