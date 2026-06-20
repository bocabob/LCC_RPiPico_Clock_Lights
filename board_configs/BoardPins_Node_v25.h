/*
 * Pin definitions for LCC RPi Pico Node board v2.5
 *
 * Do not include this file directly — it is selected automatically
 * by BoardSettings.h based on the LCC_BOARD_* define in ProjectConfig.h.
 *
 * v2.5 layout:
 *   NeoPixel outputs : gp2 (A), gp3 (D), gp6 (B), gp7 (C)
 *   I2C0 for servo   : gp12 (SDA) / gp13 (SCL)  — I/O-1:Pin4/Pin3
 *   I2C1 for storage : gp26 (SDA) / gp27 (SCL)  — Wire1
 *   CAN (SPI0)       : SDO=gp16, CS=gp17, SCK=gp18, SDI=gp19, INT=gp20
 *   Buttons          : Blue=gp21, Gold=gp22
 */

#ifndef BOARDPINS_NODE_V25_H
#define BOARDPINS_NODE_V25_H

// --------------------------------------------
//  NeoPixel outputs (dedicated pins)
// --------------------------------------------
#define NeoPixel_PinA   2
#define NeoPixel_PinB   6
#define NeoPixel_PinC   7
#define NeoPixel_PinD   3

// --------------------------------------------
//  I2C0 — servo controller (PCA9685)
//  gp12/gp13 = I/O-1:Pin4/Pin3, valid I2C0 pins on RP2040
// --------------------------------------------
#define SERVO_SDA   12
#define SERVO_SCL   13

// --------------------------------------------
//  I2C1 — EEPROM storage (Wire1)
// --------------------------------------------
#define I2C_SDA     26
#define I2C_SCL     27
#define STOR_WIRE   Wire1

// --------------------------------------------
//  CAN transceiver — MCP2517/18 via SPI0
// --------------------------------------------
#define MCP2517_SPI SPI
#define MCP2517_SDO 16   // MISO from MCP2517
#define MCP2517_CS  17
#define MCP2517_SCK 18
#define MCP2517_SDI 19   // MOSI to MCP2517
#define MCP2517_INT 20

// --------------------------------------------
//  Buttons
// --------------------------------------------
#define BLUE_BUTTON_PIN  21
#define GOLD_BUTTON_PIN  22

#endif  // BOARDPINS_NODE_V25_H
