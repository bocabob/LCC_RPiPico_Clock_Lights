# LCC_RPiPico_Clock_Lights

An OpenLCB (LCC) node running on a Raspberry Pi Pico / Pico 2 (v3.0 generic Node board)
that combines a fast clock display with NeoPixel lighting control:

- **Fast clock display** — receives the LCC broadcast fast clock and shows the current
  layout time on a 7″ 800×480 LT7381 (RA8876-compatible) display
- **NeoPixel lighting control** — controls up to four WS2811/WS2812 LED strings with
  per-pixel effects, group scheduling, and time-based on/off driven by the fast clock

This project is part of a family of node firmwares sharing a common platform — see
[`LCC_RPiPico_Common/LCC_NODE_STANDARD.md`](../LCC_RPiPico_Common/LCC_NODE_STANDARD.md)
for the cross-project conventions this README assumes, and
[`LCC_RPiPico_Common/CLAUDE.md`](../LCC_RPiPico_Common/CLAUDE.md) as the entry point for
Claude Code sessions working across the family.

**Authors:** Jim Kueneman, Bob Gamble
**Node ID range:** `05.01.01.01.94.xx` — assigned to Bob Gamble / Southern Piedmont

---

## Features

- Fast clock display: LCC broadcast time shown centred on the 7″ screen via the native
  RA8876/LT7381 hardware font
- Four independent NeoPixel LED strings, up to 20 pixels each
- Per-pixel, per-channel (R/G/B) effect configuration: off, constant, blink, flicker
- Six independently controllable groups with LCC event-based on/off
- Per-group time-based scheduling driven by the LCC broadcast fast clock
- Two global luminosity levels (full / dim) controlled by LCC events
- String-level on/off/toggle and effects on/off/toggle via LCC events
- Per-string dimmer toggle
- Configuration stored in external I2C EEPROM (non-volatile, survives power cycles)
- Configuration accessible and editable via any standard LCC configuration tool (JMRI, etc.)
- **Protected node identity** — node ID survives config wipes/EEPROM_VERSION bumps;
  provisioned via the serial `N`/`Y` command pair (Node Standard §7.1)
- **Factory reset gesture** — hold Blue+Gold for 2s at boot to wipe config to CDI
  defaults without a serial connection (§7.2)
- OTA firmware update support via LittleFS + PicoOTA
- Serial monitor debug interface for NVM management and diagnostics
- Dual-core RP2040: Core 0 runs the LCC/CAN stack, Core 1 runs pixel processing

---

## Board Support

The active hardware target is the **v3.0 generic Node board** — see the Node Standard's
§4 (Board Versioning) for the full family history; v2.5–v2.95 board revisions remain
supported in `board_configs/` but v3.0 is current.

Edit [`ProjectConfig.h`](ProjectConfig.h) — the **single source of truth** for board and
display driver selection:

```cpp
#define LCC_BOARD_NODE_V30              // v3.0 board (current generic NODE board)
#define DISPLAY_DRIVER_RA8876_NATIVE    // native RA8876_RP2040 library (LT7381 / RA8876)
```

[`NodeConfig.h`](NodeConfig.h) maps the v3.0 physical connector pins to this project's
functional signal names (NeoPixel strings, display, touch) — pin assignments are
carried over unchanged from the v2.95 board header, since I/O-1 and I/O-2 pins 1–4 are
wired identically between the two revisions.

---

## Hardware — v3.0

| Component | Details |
|---|---|
| **MCU** | Raspberry Pi Pico / Pico 2, v3.0 generic Node carrier board |
| **CAN controller** | MCP2517/2518FD on SPI0 (gp0–4) — fixed-function on the Node board |
| **EEPROM** | 24LC256 (32 KB) on I2C1 (gp6/7), address `0x50` |
| **Display** | ER-TFTMC070-4 — 7″, 800×480, LT7381 controller (RA8876-compatible), native driver |
| **Touch controller** | XPT2046 resistive, shares SPI1 with the display (defined but not actively used — display is fast-clock output only) |
| **NeoPixel** | Up to 4 strings |

### Pin Assignments (v3.0)

| Signal | GPIO | Connector |
|---|---|---|
| CAN (MCP2517/18 SPI0) | gp0–4 | fixed |
| EEPROM I2C1 (SDA/SCL) | gp6/gp7 | fixed |
| Display SDI/CS/SCK/SDO (SPI1) | gp8/9/10/11 | I/O-1 pins 1–4 |
| Touch MISO/CS (XPT2046, SPI1 alternates) | gp12/13 | I/O-1 pins 7/8 |
| Display RST | gp14 | I/O-1 pin 9 |
| NeoPixel A–D | gp16/17/18/19 | I/O-2 pins 1–4 |
| Blue Button | gp5 | I/O-2 pin 10 |
| Gold Button | gp28 | I/O-3 pin 5 |

> **NeoPixel wiring best practices:** add a 1000 µF capacitor across the strip's + and
> − supply; place a 300–500 Ω resistor in series with each data line; use a
> logic-level converter on the data line when driving 5 V strips from the 3.3 V Pico.

See [`NodeConfig.h`](NodeConfig.h) for the authoritative mapping.

---

## LCC Node Identity

The node's LCC node ID is **not** a hardcoded `#define` — it lives in a protected NVM
region that survives config wipes and `EEPROM_VERSION` bumps (Node Standard §7.1). On
an unprovisioned board it falls back to a legacy default and prints a warning:

```cpp
#define NODE_ID_DEFAULT 0x050101019400   // fallback only — provision a real ID below
```

Provision (or re-provision) a node over serial, two-step with confirmation:

```
N050101019400        → node replies "Confirm with 'Y' to write 05:01:01:01:94:00"
Y                    → node writes the identity block and reboots
```

---

## Dependencies

Install via the Arduino IDE Library Manager or manually:

| Library | Purpose |
|---|---|
| `ACAN2517` by Pierre Molinaro | MCP2517/2518 CAN transceiver driver |
| [`RA8876_RP2040`](https://github.com/wwatson4506/Ra8876LiteTeensy) | LT7381/RA8876 native display driver |
| [`NeoPixelBusLg`](https://github.com/Makuna/NeoPixelBus) by Makuna | NeoPixel control with luminance support |
| `I2C_eeprom` by Rob Tillaart | EEPROM read/write (`USE_TILLAART`) |
| `LibPrintf` | `printf()` support over Serial |
| `PicoOTA` | Over-the-air firmware update (Philhower core) |
| `LittleFS` | Flash filesystem for OTA image staging |
| `Wire`, `SPI` | Built into the Arduino core |

The OpenLCB stack (`src/openlcb/`, `src/drivers/canbus/`) is vendored as the
MustangPeak OpenLcbCLib — see it as a fixed external dependency (Node Standard §10);
do not modify files under `src/`.

> **Board package:** Use [Earle Philhower's RP2040 package](https://github.com/earlephilhower/arduino-pico#installation) — **not** the Mbed package.
> Board target: `rp2040:rp2040:rpipico2`

---

## Build Configuration

See [`sketch.yaml`](sketch.yaml) for the full build profile:

- Flash: 4 MB — 2 MB filesystem space for firmware updates
- Optimization: `Small`
- C++ standard: `gnu++17`

---

## Configuration Memory (CDI)

Node configuration (string count, luminosity levels, effect frequency, group/string
event IDs, etc.) is stored in external EEPROM and described by [`CDI.xml`](CDI.xml).
Edit it with any LCC configuration tool (e.g. JMRI's PanelPro) over the LCC bus.

`openlcb_user_config.c`'s compiled `_cdi_data[]` byte array must be kept in sync with
`CDI.xml` by hand any time the XML changes — regenerate it with
[`LCC_RPiPico_Common/cdi_to_c_array.py`](../LCC_RPiPico_Common/cdi_to_c_array.py)
rather than hand-editing the array (see the Node Standard §7 for the exact splicing
procedure).

### Consumed Events (registration order)

| Index | Event |
|---|---|
| 0 | Full luminosity on |
| 1 | Low luminosity on |
| 2–13 | Groups 0–5 on/off (pairs) |
| 14–41 | Strings 0–3 × 7 events (all-on, all-off, all-toggle, effects-on, effects-off, effects-toggle, dimmer-toggle) |

---

## LED Effects

Each LED lead (R, G, or B channel) is independently configured:

| Field | Description |
|---|---|
| `intensity` | Channel brightness (0–255) |
| `effect` | 0 = off, 1 = constant, 2 = blink, 3 = flicker |
| `cycles_on` | Effect cycles to stay on (blink/flicker) |
| `cycles_off` | Effect cycles to stay off (blink/flicker) |
| `starting_cycle` | Initial state for blink/flicker |
| `group` | Group number (0–5) that gates this channel |

Effect cycle period is set by `effect_cycle_frequency` in the Attributes segment
(default 100 ms).

---

## Serial CLI Commands

Connect at **115200 baud**. Commands common across the node family (Node Standard §11)
plus this project's own:

| Key | Action |
|---|---|
| `h` | Print help |
| `c` | Clear NVM to `0x00` |
| `i` | Reset NVM to CDI default values |
| `r` | Factory reset (NVM to `0xFF`, then reinitialize) |
| `p` | Toggle LCC message logging |
| `m` | Toggle config memory read/write logging |
| `t` | Display current fast clock time |
| `q` | Send a fast clock query on the LCC bus |
| `N` | Provision/re-provision node identity — two-step, confirm with `Y` |

---

## Architecture

The Pico runs two cores (Node Standard §8 dual-core contract):

- **Core 0** (`setup()` / `loop()`): OpenLCB protocol stack, CAN comms, event handling,
  serial CLI
- **Core 1** (`setup1()` / `loop1()`): NeoPixel effect processing — never blocks on
  `delay()`, EEPROM I/O, or CAN traffic

Key modules:

| File | Role |
|---|---|
| `LCC_RPiPico_Clock_Lights.ino` | Entry point: `setup()`/`loop()` (LCC, Core 0), `loop1()` (pixels, Core 1) |
| `ProjectConfig.h` | Single switch: board macro + display driver macro |
| `BoardSettings.h` | Dispatches to the matching `board_configs/` header; NVM/storage sizing; global tuning constants |
| `NodeConfig.h` | Functional pin layer: NeoPixel strings, display geometry, touch pin mapping |
| `board_configs/BoardPins_Node_v30.h` | v3.0 physical pin topology (current) |
| `board_configs/BoardPins_Node_v25/26/27/28/295.h` | Earlier Node-family pin topology (still supported) |
| `ClockDisplay.h` / `.cpp` | Fast-clock rendering (`setupDisplay()`, `drawFastClock()`) |
| `DisplayDriver.h` / `.cpp` | `TT_Display` wrapper around `RA8876_RP2040` (LT7381-compatible) |
| `NPlights.h` / `.cpp` | NeoPixel state machine and effect processing |
| `callbacks.cpp` | OpenLCB event handlers, 100ms timer, CAN Rx/Tx, OTA firmware — the only place consumers/producers are registered (Node Standard §10) |
| `config_mem_helper.cpp` | EEPROM config storage, CDI memory map |
| `NodeIdentity.h` / `.cpp` | Protected-NVM node identity block (Node Standard §7.1) |

---

## License

BSD 2-Clause. Copyright © 2026 Jim Kueneman & Bob Gamble. See source file headers for
full license text (no separate `LICENSE` file in this repo).
