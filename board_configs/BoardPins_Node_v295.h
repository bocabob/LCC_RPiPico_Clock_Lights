/*
 *  Pin definitions for LCC Node board v2.95
 *  Used in the Clock_Lights project with:
 *    - Four NeoPixel strings on I/O-2 (gp16-19)
 *    - LT7381 (RA8876 clone) 800×480 display on I/O-1 (SPI1, gp8-14)
 *    - XPT2046 resistive touch (SPI1 shared, T_CS=gp12, T_IRQ=gp13)
 *
 *  Do not include this file directly — it is selected automatically
 *  by BoardSettings.h based on LCC_BOARD_NODE_V295 in ProjectConfig.h.
 *
 *  I/O-1 10-pin IDC connector mapping (display breakout attaches here):
 *    Pin 1  gp8   DISPLAY_SDI — SPI1 RX  (display SDO → RP2040)
 *    Pin 2  gp9   DISPLAY_CS
 *    Pin 3  gp10  DISPLAY_SCK
 *    Pin 4  gp11  DISPLAY_SDO — SPI1 TX  (RP2040 → display SDI)
 *    Pin 5  GND
 *    Pin 6  3.3 V  (also powers D_BL backlight via breakout)
 *    Pin 7  gp12  TOUCH_MISO — XPT2046 DOUT (SPI1 RX alternate; switched per-transaction)
 *    Pin 8  gp13  TOUCH_CS   — XPT2046 T_CS (SPI1 CSn alternate pin)
 *    Pin 9  gp14  DISPLAY_RST
 *    Pin 10 GND
 *
 *  XPT2046 touch SPI lines — SPI1 shared with display, MISO switched per-transaction:
 *    T_CLK  → gp10 (DISPLAY_SCK  — shared)
 *    T_DIN  → gp11 (DISPLAY_SDO / SPI1 TX — shared)
 *    T_DOUT → gp12 (TOUCH_MISO  — SPI1 RX alternate; display uses gp8)
 *    T_CS   → gp13 (TOUCH_CS    — SPI1 CSn alternate — I/O-1:Pin8)
 *
 *  Per-transaction: SPI1.setRX(gp12) for touch reads, SPI1.setRX(gp8) for display.
 *  CAN (SPI0) uses gp0-4 — no SPI conflict with display SPI1.
 */

#ifndef BOARDPINS_NODE_V295_H
#define BOARDPINS_NODE_V295_H

// --------------------------------------------
//  NeoPixel strings — I/O-2:Pin1-4
// --------------------------------------------
#define NeoPixel_PinA   16  // I/O-2:Pin1
#define NeoPixel_PinB   17  // I/O-2:Pin2
#define NeoPixel_PinC   18  // I/O-2:Pin3
#define NeoPixel_PinD   19  // I/O-2:Pin4

// --------------------------------------------
//  I2C1 — EEPROM storage (Wire1, gp6/gp7)
// --------------------------------------------
#define I2C_SDA     6
#define I2C_SCL     7
#define STOR_WIRE   Wire1

// --------------------------------------------
//  CAN transceiver — MCP2517/18 via SPI0 (gp0-4)
// --------------------------------------------
#define MCP2517_SPI SPI
#define MCP2517_SDO 0   // MISO from MCP2517 (ACAN_RX_PIN)
#define MCP2517_CS  1   // ACAN_CS_PIN
#define MCP2517_SCK 2   // ACAN_CSK_PIN
#define MCP2517_SDI 3   // MOSI to MCP2517  (ACAN_TX_PIN)
#define MCP2517_INT 4   // ACAN_INT_PIN

// --------------------------------------------
//  Buttons
// --------------------------------------------
#define BLUE_BUTTON_PIN     21  // I/O-2:Pin8
#define GOLD_BUTTON_PIN     22  // I/O-2:Pin9

// --------------------------------------------
//  Display geometry & orientation
//  ER-TFTMC070-4 7" LT7381 (RA8876 clone), 800×480
// --------------------------------------------
#define HRES           800
#define VRES           480
#define ROTATION          0
#define TOUCH_ROTATION    0

// --------------------------------------------
//  Touch — XPT2046 resistive (SPI1 shared)
//    T_CLK / T_DIN share gp10 / gp11 with the display (shared SPI1 TX/SCK).
//    T_DOUT uses a *separate* SPI1 RX alternate pin (gp12) so SPI1.setRX()
//    can switch MISO between the display (gp8) and touch (gp12) per-transaction.
//    T_CS uses gp13 (SPI1 CSn alternate) — independent of DISPLAY_CS (gp9).
// --------------------------------------------
#define TOUCH_CS    13  // I/O-1:Pin8  — XPT2046 T_CS  (SPI1 CSn alternate)
#define TOUCH_MISO  12  // I/O-1:Pin7  — XPT2046 DOUT  (SPI1 RX alternate; display uses gp8)

// --------------------------------------------
//  Display — LT7381 (RA8876 clone) via SPI1 on I/O-1
//
//  Naming from the RP2040's perspective:
//    DISPLAY_SDO = RP2040 serial data OUT (SPI1 TX, gp11) → display SDI
//    DISPLAY_SDI = RP2040 serial data IN  (SPI1 RX, gp8)  ← display SDO
// --------------------------------------------
#define DISPLAY_SPI     SPI1
#define DISPLAY_SDI      8   // I/O-1:Pin1 — SPI1 RX  (MISO ← display)
#define DISPLAY_CS       9   // I/O-1:Pin2
#define DISPLAY_SCK     10   // I/O-1:Pin3
#define DISPLAY_SDO     11   // I/O-1:Pin4 — SPI1 TX  (MOSI → display)
#define DISPLAY_RST     14   // I/O-1:Pin9

#endif  // BOARDPINS_NODE_V295_H
