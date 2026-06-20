# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

See [../LCC_RPiPico_Common/LCC_NODE_STANDARD.md](../LCC_RPiPico_Common/LCC_NODE_STANDARD.md) for cross-project conventions (toolchain, board versioning, dual-core contract, CDI/EEPROM handling, naming). This file documents only what is specific to this node.

## Build Environment

- **Board family/revision**: Node board family — `LCC_BOARD_NODE_V295` is current (`ProjectConfig.h`); v2.5/v2.6/v2.7/v2.8 supported
- **Display driver**: `DISPLAY_DRIVER_RA8876_NATIVE` (LT7381 7" 800×480, register-compatible with RA8876)
- No project-specific libraries beyond the family standard (`ACAN2517`, `I2C_eeprom`, `NeoPixelBus`, `Wire`)

## Architecture

This is an OpenLCB (LCC) node combining a fast-clock display (LCC broadcast time on a 7" 800×480 LT7381 display) with NeoPixel lighting control (up to four WS2811/WS2812 strings, per-pixel effects, group scheduling driven by the same fast clock).

### Key Module Responsibilities

| File | Role |
|---|---|
| [LCC_RPiPico_Clock_Lights.ino](LCC_RPiPico_Clock_Lights.ino) | Entry point, node init, consumer/producer registration, serial CLI |
| [ClockDisplay.cpp](ClockDisplay.cpp) | Renders fast-clock time to the display |
| [DisplayDriver.cpp](DisplayDriver.cpp) | Low-level display driver glue |
| [NPlights.cpp](NPlights.cpp) | NeoPixel string/group effects, scheduling, luminosity |
| [callbacks.cpp](callbacks.cpp) | OpenLCB event handlers (consumed/produced), fast clock receive, 100ms timer |
| [config_mem_helper.cpp](config_mem_helper.cpp) | I2C EEPROM config storage, CDI-driven memory map |
| [BoardSettings.h](BoardSettings.h) | All hardware pin and storage definitions |
| `nixie_images.h` / `pragotron_images.h` | Bitmap assets for alternate clock face styles |

### Key Data Flow

1. **Fast clock receive**: LCC broadcast fast-clock event → `callbacks.cpp` → `ClockDisplay.cpp` updates rendered time
2. **NeoPixel scheduling**: fast-clock time → `NPlights.cpp` group schedule check → on/off per group
3. **LCC lighting control**: consumed event (string/group on/off/toggle, luminosity) → `callbacks.cpp` → `NPlights.cpp`

### Important Implementation Notes

- Dual-core: Core 0 runs LCC/CAN stack and display; Core 1 runs NeoPixel pixel processing
- v2.95 board pairs NeoPixel on I/O-2 (gp16-19) with the LT7381 display + XPT2046 touch on I/O-1 (gp8-14) — see `board_configs/BoardPins_Node_v295.h` if/when added for this family
- Node ID range `05.01.01.01.94.xx` (Bob Gamble / Southern Piedmont)

**Protected NVM identity block** (standard §7.1) is implemented:
`NodeIdentity.h`/`.cpp` read/write a 12-byte block above `CONFIG_MEM_SIZE`
(now `32704`, down from `32768`, freeing 64 reserved bytes). On boot, if the
block isn't provisioned, the node falls back to the legacy hardcoded
`NODE_ID_DEFAULT` for that session and logs a warning — it does not halt.
Provision a permanent ID with the serial `'N<12 hex digits>'` + `'Y'` confirm
commands.

`LCC_BOARD_NODE_V30` (generic v3.0 Node board) is now selectable in
`ProjectConfig.h`.
