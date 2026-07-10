/*
 * NodeConfig.h — functional pin assignment layer for the v3.0 generic
 * NODE board (LCC_BOARD_NODE_V30). Maps the physical IOx_PINy connector
 * pins from board_configs/BoardPins_Node_v30.h to this project's functional
 * signal names (NeoPixel strings, display, touch).
 *
 * Only included when LCC_BOARD_NODE_V30 is active (see BoardSettings.h).
 * Pin assignments below are carried over unchanged from
 * board_configs/BoardPins_Node_v295.h — I/O-1 and I/O-2 Pin1-4 are wired
 * identically between v2.95 and v3.0, only I/O-2 Pin10 and the button pins
 * moved (see BoardPins_Node_v30.h's header comment).
 *
 * BLUE_BUTTON_PIN / GOLD_BUTTON_PIN are NOT redefined here — v3.0's board
 * header already defines them (gp5/gp28); redefining them here would be a
 * macro-redefinition error.
 */

#ifndef NODECONFIG_H
#define NODECONFIG_H

// --------------------------------------------
//  NeoPixel strings — I/O-2:Pin1-4
// --------------------------------------------
#define NeoPixel_PinA   IO2_PIN1   // gp16
#define NeoPixel_PinB   IO2_PIN2   // gp17
#define NeoPixel_PinC   IO2_PIN3   // gp18
#define NeoPixel_PinD   IO2_PIN4   // gp19

// --------------------------------------------
//  Display geometry & orientation
//  ER-TFTMC070-4 7" LT7381 (RA8876 clone), 800×480
// --------------------------------------------
#define HRES           800
#define VRES           480
#define ROTATION          0
#define TOUCH_ROTATION    0

// --------------------------------------------
//  Touch — XPT2046 resistive (SPI1 shared with display)
//    T_CLK / T_DIN share I/O-1 Pin3/Pin4 with the display (shared SPI1 TX/SCK).
//    T_DOUT uses a *separate* SPI1 RX alternate pin so SPI1.setRX() can
//    switch MISO between the display and touch per-transaction.
//    T_CS is independent of DISPLAY_CS.
// --------------------------------------------
#define TOUCH_CS    IO1_PIN8   // gp13 — XPT2046 T_CS  (SPI1 CSn alternate)
#define TOUCH_MISO  IO1_PIN7   // gp12 — XPT2046 DOUT  (SPI1 RX alternate; display uses gp8)

// --------------------------------------------
//  Display — LT7381 (RA8876 clone) via SPI1 on I/O-1
//
//  Naming from the RP2040's perspective:
//    DISPLAY_SDO = RP2040 serial data OUT (SPI1 TX) → display SDI
//    DISPLAY_SDI = RP2040 serial data IN  (SPI1 RX) ← display SDO
// --------------------------------------------
#define DISPLAY_SPI     SPI1
#define DISPLAY_SDI     IO1_PIN1   // gp8  — SPI1 RX  (MISO ← display)
#define DISPLAY_CS      IO1_PIN2   // gp9
#define DISPLAY_SCK     IO1_PIN3   // gp10
#define DISPLAY_SDO     IO1_PIN4   // gp11 — SPI1 TX  (MOSI → display)
#define DISPLAY_RST     IO1_PIN9   // gp14

#endif  // NODECONFIG_H
