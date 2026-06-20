# LCC RPi Pico Clock & Lights

An OpenLCB (LCC) node for the Raspberry Pi Pico 2 (RP2040) that combines:

- **Fast clock display** — receives the LCC broadcast fast clock and shows the current layout time on a 7″ 800×480 LT7381 (RA8876-compatible) display
- **NeoPixel lighting control** — controls up to four WS2811/WS2812 LED strings with per-pixel effects, group scheduling, and time-based on/off driven by the fast clock

**Authors:** Jim Kueneman, Bob Gamble  
**Node ID range:** `05.01.01.01.94.xx` — assigned to Bob Gamble / Southern Piedmont

---

## Features

- Fast clock display: LCC broadcast time shown centred on the 7″ screen in a 64×128 px hardware font (≈ 325×128 px for "HH:MM")
- Four independent NeoPixel LED strings, up to 20 pixels each
- Per-pixel, per-channel (R/G/B) effect configuration: off, constant, blink, flicker
- Six independently controllable groups with LCC event-based on/off
- Per-group time-based scheduling driven by the LCC broadcast fast clock
- Two global luminosity levels (full / dim) controlled by LCC events
- String-level on/off/toggle and effects on/off/toggle via LCC events
- Per-string dimmer toggle
- Configuration stored in external I2C EEPROM (non-volatile, survives power cycles)
- Configuration accessible and editable via any standard LCC configuration tool (JMRI, etc.)
- OTA firmware update support via LittleFS + PicoOTA
- Serial monitor debug interface for NVM management and diagnostics
- Dual-core RP2040: Core 0 runs the LCC/CAN stack, Core 1 runs pixel processing

---

## Board Support

Select the active board in **`ProjectConfig.h`** (one `#define` only):

| Define | Board | Notes |
|--------|-------|-------|
| `LCC_BOARD_NODE_V25` | v2.5 | NeoPixel gp2/3/6/7, CAN gp16-20, I2C1 gp26/27 |
| `LCC_BOARD_NODE_V26` | v2.6 | Identical pin assignments to v2.5 |
| `LCC_BOARD_NODE_V27` | v2.7 | All I/O headers, no dedicated NeoPixel, CAN gp16-20 |
| `LCC_BOARD_NODE_V28` | v2.8 | CAN gp0-4, I2C1 gp6/7, I2C0 gp16/17 |
| **`LCC_BOARD_NODE_V295`** | **v2.95** | **Current — CAN gp0-4, NeoPixel I/O-2, LT7381 display I/O-1** |

The display driver (`DISPLAY_DRIVER_RA8876_NATIVE`) applies only to the v2.95 board and is set automatically in `ProjectConfig.h`.

---

## Hardware — Node Board v2.95

### CAN Interface — MCP2517/2518 (SPI0, gp0–4)

| Signal | GPIO |
|--------|------|
| SDO (MISO from MCP) | gp0 |
| CS | gp1 |
| SCK | gp2 |
| SDI (MOSI to MCP) | gp3 |
| INT | gp4 |
| SPI bus | SPI0 (`SPI`) |

### I2C EEPROM — Configuration Memory (Wire1, gp6/7)

| Signal | GPIO |
|--------|------|
| SDA | gp6 |
| SCL | gp7 |
| Address | 0x50 |
| Bus | Wire1 (`STOR_WIRE`) |

Supported device: 24LC256 (32 KB). Adjust `I2C_DEVICESIZE` and `CONFIG_MEM_SIZE` in `BoardSettings.h` to match a different chip.

### NeoPixel Output — I/O-2 Connector (gp16–19)

| String | GPIO | I/O-2 Pin |
|--------|------|-----------|
| A | gp16 | Pin 1 |
| B | gp17 | Pin 2 |
| C | gp18 | Pin 3 |
| D | gp19 | Pin 4 |

Strings use the RP2040 PIO hardware via `NeoPixelBusLg`. Default method targets WS2811; change to `Rp2040x4Pio1Ws2812xMethod` in `NPlights.cpp` for WS2812 strips.

> **NeoPixel wiring best practices:**  
> Add a 1000 µF capacitor across the strip's + and − supply.  
> Place a 300–500 Ω resistor in series with each data line.  
> Use a logic-level converter on the data line when driving 5 V strips from the 3.3 V Pico.

### Display — LT7381 (RA8876 clone) via I/O-1 (SPI1, gp8–14)

Display module: **ER-TFTMC070-4** — 7″, 800×480, LT7381 controller (RA8876-compatible).

| Signal | GPIO | I/O-1 Pin |
|--------|------|-----------|
| SDO — SPI1 RX (display → RP2040) | gp8 | Pin 1 |
| CS | gp9 | Pin 2 |
| SCK | gp10 | Pin 3 |
| SDI — SPI1 TX (RP2040 → display) | gp11 | Pin 4 |
| RST | gp14 | Pin 9 |
| GND | — | Pin 5 |
| 3.3 V (also powers backlight via breakout) | — | Pin 6 |

The display driver is the native `RA8876_RP2040` library. Timing values in `DisplayDriver.h → TT_Display::init()` are set for this 800×480 panel; if the image appears shifted or has sync artifacts on first use, adjust the `lcdHorizontal*` / `lcdVertical*` parameters to match the panel datasheet.

### Resistive Touch — XPT2046 via shared SPI1 (gp8/10/11 + gp12/13)

The XPT2046 shares the display's SPI1 data lines (MISO/SCK/MOSI) and has its own chip-select line.

| Signal | GPIO | I/O-1 Pin | Notes |
|--------|------|-----------|-------|
| T_DOUT (MISO, shared with display) | gp8 | Pin 1 | Same wire as D_SDO |
| T_SCLK (SCK, shared with display) | gp10 | Pin 3 | Same wire as D_CLK |
| T_DIN (MOSI, shared with display) | gp11 | Pin 4 | Same wire as D_SDI |
| T_CS (dedicated) | gp13 | Pin 8 | SPI1 CSn alternate pin |
| gp12 | — | Pin 7 | SPI1 RX alternate — reserved, NC |

> Touch is defined but not actively used by the Clock_Lights firmware — the display is read-only (fast clock output only). The pins are reserved for future use.

### Buttons

| Button | GPIO | I/O-2 Pin |
|--------|------|-----------|
| Blue | gp21 | Pin 8 |
| Gold | gp22 | Pin 9 |

---

## Software Prerequisites

Install via the Arduino IDE Library Manager or manually:

| Library | Purpose |
|---------|---------|
| [arduino-pico](https://github.com/earlephilhower/arduino-pico) by Earl F. Philhower | RP2040 board support — **use this, not the Arduino Mbed board** |
| [RA8876_RP2040](https://github.com/wwatson4506/Ra8876LiteTeensy) | LT7381/RA8876 display driver (RA8876-compatible) |
| [NeoPixelBus](https://github.com/Makuna/NeoPixelBus) by Makuna | NeoPixel control with luminance support |
| [LibPrintf](https://github.com/embeddedartistry/arduino-printf) | `printf` support for Arduino |
| [I2C_EEPROM](https://github.com/RobTillaart/I2C_EEPROM) by Rob Tillaart | I2C EEPROM read/write (selected via `USE_TILLAART` in `BoardSettings.h`) |
| PicoOTA | OTA firmware update (bundled with the Philhower board library) |
| LittleFS | Filesystem for OTA image staging (bundled with the Philhower board library) |

The OpenLCB stack is included in the `src/` directory and does not require a separate installation.

---

## Quick Start

1. Open `ProjectConfig.h` and confirm `LCC_BOARD_NODE_V295` is selected.
2. Set the node ID in `LCC_RPiPico_Clock_Lights.ino` (`#define NODE_ID`).
3. Flash the sketch. On first boot the EEPROM is initialised automatically.
4. Connect to the LCC bus; the node will query the fast clock and display the time.

---

## Configuration

### `ProjectConfig.h`

The single file to edit when switching hardware targets. Contains board selection (`LCC_BOARD_NODE_V*`) and display driver selection (`DISPLAY_DRIVER_RA8876_NATIVE`).

### `BoardSettings.h`

All compile-time hardware options (do not set board version here — use `ProjectConfig.h`):

| Setting | Default | Description |
|---------|---------|-------------|
| `USE_I2C_STORAGE` | enabled | Use external I2C EEPROM for config memory |
| `EXTERNAL_EEPROM` | enabled | EEPROM chip type (vs. `EXTERNAL_FRAM`) |
| `USE_TILLAART` | enabled | Use Rob Tillaart's EEPROM library |
| `CONFIG_MEM_SIZE` | 32768 | Config memory size in bytes |
| `I2C_DEVICESIZE` | 32768 | EEPROM chip capacity (24LC256 = 32768) |
| `MAX_STRINGS` | 4 | Maximum number of NeoPixel strings |
| `MAX_LIGHTS` | 20 | Maximum LEDs per string |
| `MAX_LUMINANCE` | 100 | Full-brightness luminance level |
| `EEPROM_VERSION` | 8 | Config schema version |

---

## LCC Configuration Memory Layout

Stored in address space `0xFD` (space 253), starting at address `0x00`. Layout defined by `CDI.xml` and maps directly to `config_mem_t` in `config_mem_helper.h`.

| Segment | Address | Contents |
|---------|---------|----------|
| Node ID | 0x00 | Node name (62 bytes) and description (63 bytes) |
| Reset Control | 0x7D | Reset-on-boot flag |
| Attributes | 0x7E | String count, luminosity levels, effect frequency, global luminosity event IDs |
| Controls | 0x92 | 6 groups × (on event, off event, on time HH:MM, off time HH:MM) |
| Strings | 0x10A | 4 strings × (description, head count, 7 event IDs, 20 pixels × 3 leads × 6 fields) |

### Consumed Events (registration order)

| Index | Event |
|-------|-------|
| 0 | Full luminosity on |
| 1 | Low luminosity on |
| 2–13 | Groups 0–5 on/off (pairs) |
| 14–41 | Strings 0–3 × 7 events (all-on, all-off, all-toggle, effects-on, effects-off, effects-toggle, dimmer-toggle) |

---

## Fast Clock Display

The node subscribes to the LCC default fast clock (`BROADCAST_TIME_ID_DEFAULT_FAST_CLOCK`). Each time-changed event calls `drawFastClock()` in `ClockDisplay.cpp`, which:

1. Erases the previous time with a filled black rectangle.
2. Renders `HH:MM` centred on the 800×480 screen using the LT7381 hardware font at 4× scale (64×128 px per character, ~325×128 px total).

The display position is pre-computed in `setupDisplay()` using `tft.textWidth()` and `tft.fontHeight()` and stored in module-static variables so each update is a single erase + draw call.

---

## LED Effects

Each LED lead (R, G, or B channel) is independently configured:

| Field | Description |
|-------|-------------|
| `intensity` | Channel brightness (0–255) |
| `effect` | 0 = off, 1 = constant, 2 = blink, 3 = flicker |
| `cycles_on` | Effect cycles to stay on (blink/flicker) |
| `cycles_off` | Effect cycles to stay off (blink/flicker) |
| `starting_cycle` | Initial state for blink/flicker |
| `group` | Group number (0–5) that gates this channel |

Effect cycle period is set by `effect_cycle_frequency` in the Attributes segment (default 100 ms).

---

## First Boot / NVM Initialization

On first power-up after a fresh firmware flash the EEPROM contains `0xFF`. The sketch detects this and automatically initialises the EEPROM with CDI default values, then registers all consumer event IDs with the LCC stack.

To manually reinitialize NVM use the serial monitor commands below.

---

## Serial Monitor Debug Interface

Connect at **115200 baud**:

| Key | Action |
|-----|--------|
| `h` | Print help |
| `c` | Clear all NVM to `0x00` |
| `i` | Write CDI default values to NVM |
| `r` | Reset NVM to `0xFF` (simulates factory-fresh EEPROM) |
| `p` | Toggle LCC message logging (prints CAN frames as GridConnect strings) |
| `m` | Toggle config memory read/write logging |
| `t` | Display current fast clock time |
| `q` | Send a fast clock query on the LCC bus |

---

## Firmware Updates (OTA)

Firmware updates are supported via the LCC Memory Configuration protocol (firmware address space). A configuration tool (e.g. JMRI) uploads a new `.bin` image which is staged to LittleFS as `bootloader.bin`. On the next reboot PicoOTA applies the update.

---

## Project Structure

```
LCC_RPiPico_Clock_Lights/
├── LCC_RPiPico_Clock_Lights.ino  # Main sketch: setup/loop (LCC, Core 0), loop1 (pixels, Core 1)
├── ProjectConfig.h               # Board and display driver selection — edit this first
├── BoardSettings.h               # Compile-time hardware options; includes board pin header
├── board_configs/
│   ├── BoardPins_Node_v295.h     # Active board: v2.95 pin assignments
│   └── BoardPins_Node_v*.h       # Other supported board pin files
├── ClockDisplay.h / .cpp         # LT7381 fast-clock rendering (setupDisplay, drawFastClock)
├── DisplayDriver.h / .cpp        # TT_Display wrapper around RA8876_RP2040 (LT7381-compatible)
├── NPlights.h / .cpp             # NeoPixel state machine and effect processing
├── callbacks.h / .cpp            # LCC event and protocol callbacks
├── config_mem_helper.h / .cpp    # Config memory struct, NVM read/write helpers
├── openlcb_user_config.h / .c    # Node parameters and CDI binary data
├── can_user_config.h             # CAN buffer depth configuration
├── mdebugging.h                  # Conditional debug print macros
├── CDI.xml                       # Human-readable CDI for reference / tooling
├── sketch.yaml                   # Arduino CLI build descriptor (Pico 2, 4 MB flash)
└── src/                          # OpenLCB C library (do not modify)
```

---

## License

BSD 2-Clause. Copyright © 2026 Jim Kueneman & Bob Gamble. See source file headers for full license text.
