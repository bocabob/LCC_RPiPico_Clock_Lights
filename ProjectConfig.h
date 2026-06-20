/*
 * ProjectConfig.h — THE single file to edit when switching hardware targets.
 *
 * Step 1: Uncomment exactly ONE board line.
 * Step 2: For v2.95, confirm the display driver line matches your hardware.
 *
 *  LCC_BOARD_NODE_V25   →  v2.5 board  (NeoPixel gp2/3/6/7, CAN gp16-20, I2C1 gp26/27)
 *  LCC_BOARD_NODE_V26   →  v2.6 board  (identical pin assignments to v2.5)
 *  LCC_BOARD_NODE_V27   →  v2.7 board  (all I/O headers, no dedicated NeoPixel, CAN gp16-20)
 *  LCC_BOARD_NODE_V28   →  v2.8 board  (CAN gp0-4, I2C1 gp6/7, I2C0 gp16/17)
 *  LCC_BOARD_NODE_V295  →  v2.95 board (CAN gp0-4, NeoPixel I/O-2 gp16-19,
 *                                        LT7381 display + XPT2046 touch on I/O-1 gp8-14)
 *  LCC_BOARD_NODE_V30   →  v3.0 board (CAN gp0-4, I2C1 gp6/7, IO2_PIN10 moved
 *                                        to gp5, Blue/Gold buttons on gp5/gp28)
 */

// ---- Step 1: Board selection -----------------------------------------------
// #define LCC_BOARD_NODE_V25    // v2.5 board
// #define LCC_BOARD_NODE_V26    // v2.6 board (identical pins to v2.5)
// #define LCC_BOARD_NODE_V27    // v2.7 board (no dedicated NeoPixel pins)
// #define LCC_BOARD_NODE_V28    // v2.8 board (CAN/I2C layout changed)
#define LCC_BOARD_NODE_V295      // v2.95 board (NeoPixel I/O-2, LT7381 display + XPT2046 touch I/O-1)
//#define LCC_BOARD_NODE_V30      // v3.0 board (current generic NODE board)

// ---- Step 2: Display driver (v2.95 and any board with I/O-1 display header) -
// Any board that defines DISPLAY_CS (and the other DISPLAY_* pin constants) in
// its board_configs header can drive a display.  Boards without those pin
// defines compile no-op stubs automatically — no change needed here.
// The ER-TFTMC070-4 (7" 800×480) uses an LT7381 controller which is
// register-compatible with the RA8876, so the native library works unchanged.
#define DISPLAY_DRIVER_RA8876_NATIVE    // native RA8876_RP2040 library (LT7381 / RA8876)
