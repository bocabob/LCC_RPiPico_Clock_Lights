/*
 *  DisplayDriver.h — RA8876 display abstraction for LCC RPi Pico Clock_Lights
 *
 *  Supports DISPLAY_DRIVER_RA8876_NATIVE (RA8876_RP2040 library).
 *  Board pin defines (DISPLAY_CS/RST/SDI/SCK/SDO) come from the active
 *  board_configs/BoardPins_*.h via BoardSettings.h.
 */

#pragma once

#include <Arduino.h>
#include "BoardSettings.h"

#if defined(DISPLAY_DRIVER_RA8876_NATIVE) && defined(DISPLAY_CS)

#include <SPI.h>
#include <RA8876_RP2040.h>

// ---------------------------------------------------------------------------
//  Color constants — RGB565
// ---------------------------------------------------------------------------
#ifndef TFT_BLACK
  #define TFT_BLACK       0x0000
  #define TFT_NAVY        0x000F
  #define TFT_DARKGREEN   0x03E0
  #define TFT_DARKCYAN    0x03EF
  #define TFT_MAROON      0x7800
  #define TFT_PURPLE      0x780F
  #define TFT_OLIVE       0x7BE0
  #define TFT_LIGHTGREY   0xD69A
  #define TFT_DARKGREY    0x7BEF
  #define TFT_BLUE        0x001F
  #define TFT_GREEN       0x07E0
  #define TFT_CYAN        0x07FF
  #define TFT_RED         0xF800
  #define TFT_MAGENTA     0xF81F
  #define TFT_YELLOW      0xFFE0
  #define TFT_WHITE       0xFFFF
  #define TFT_ORANGE      0xFDA0
  #define TFT_GREENYELLOW 0xB7E0
  #define TFT_GOLD        0xFEA0
#endif

// ---------------------------------------------------------------------------
//  Text datum constants
// ---------------------------------------------------------------------------
#ifndef TL_DATUM
  #define TL_DATUM   0
  #define TC_DATUM   1
  #define TR_DATUM   2
  #define ML_DATUM   3
  #define MC_DATUM   4
  #define MR_DATUM   5
  #define BL_DATUM   6
  #define BC_DATUM   7
  #define BR_DATUM   8
#endif

// ---------------------------------------------------------------------------
//  TT_Display — TFT_eSPI-compatible wrapper around RA8876_RP2040
// ---------------------------------------------------------------------------
class TT_Display : public RA8876_RP2040 {
public:
    TT_Display(uint8_t cs, uint8_t rst, uint8_t mosi, uint8_t sclk, uint8_t miso)
        : RA8876_RP2040(cs, rst, mosi, sclk, miso) {}

    // Initialise display; returns true on success.
    // Timing is for the ER-TFTMC070-4 7" 800×480 LT7381 (RA8876 clone) panel.
    // If the image appears shifted or shows sync artifacts, adjust the five
    // horizontal/vertical timing parameters to match your panel's datasheet.
    bool init() {
        if (!RA8876_RP2040::begin(20000000))  // 20 MHz — safe for breadboard; try 40 MHz if stable
            return false;
        lcdRegDataWrite(0x13, 0xC0);
        lcdHorizontalWidthVerticalHeight(800, 480);
        lcdHorizontalNonDisplay(88);   // HNDP — horizontal blank (back porch region)
        lcdHsyncStartPosition(40);     // HSTR — HSYNC start within blank period
        lcdHsyncPulseWidth(48);        // HPW  — HSYNC pulse width
        lcdVerticalNonDisplay(32);     // VNDP — vertical blank (back porch region)
        lcdVsyncStartPosition(13);     // VSTR — VSYNC start within blank period
        lcdVsyncPulseWidth(3);         // VPW  — VSYNC pulse width
        displayOn(true);
        return true;
    }

    void    setRotation(uint8_t r) { _rot = r; RA8876_RP2040::setRotation(r); }
    uint8_t getRotation()          { return _rot; }

    void setTextColor(uint16_t fg, uint16_t bg = 0x0000) {
        _fg = fg; _bg = bg;
        RA8876_RP2040::setTextColor(fg, bg);
    }

    void    setTextDatum(uint8_t datum) { _datum = datum; }
    uint8_t getTextDatum()              { return _datum; }
    void    setTextPadding(int16_t pad) { _padding = pad; }

    void setCursor(int32_t x, int32_t y) {
        RA8876_RP2040::setCursor((int16_t)x, (int16_t)y);
    }
    void    setCursor(int32_t x, int32_t y, uint8_t font);
    int16_t getCursorY() { return RA8876_RP2040::getCursorY(); }

    int16_t textWidth(const char* str, uint8_t font);
    void    drawString(const char* str, int32_t x, int32_t y, uint8_t font);
    int16_t fontHeight(uint8_t font) { return _fontHeight(font); }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

    // Blast a packed RGB565 pixel array into a rectangle.
    // data may be in PROGMEM (flash) — transfer is chunked through a RAM buffer
    // so it is safe for DMA reads from the XIP flash region on RP2040.
    void drawRGB565(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data);

    // Blit a 1-bit packed monochrome image (MSB-first, 8px/byte, row-major).
    // Pixel=1 → fg_color; Pixel=0 → bg_top (rows < split_y) or bg_bot (rows >= split_y).
    // A 4-pixel-tall horizontal band at split_y is filled with split_color.
    // Suitable for Pragotron-style tiles: white digits on a dark split-card background.
    void drawBitmask1bit(int16_t x, int16_t y, int16_t w, int16_t h,
                         const uint8_t* bits,
                         uint16_t fg,
                         uint16_t bg_top, uint16_t bg_bot,
                         int16_t split_y, uint16_t split_color);
    void fillTriangle(int16_t x0, int16_t y0,
                      int16_t x1, int16_t y1,
                      int16_t x2, int16_t y2, uint16_t color);
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void drawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
    void drawSmoothRoundRect(int32_t x, int32_t y,
                             int32_t r, int32_t ir,
                             int32_t w, int32_t h,
                             uint32_t fg_color,
                             uint32_t bg_color = 0x00FFFFFF,
                             uint8_t  quadrants = 0xF);
    void drawSmoothCircle(int32_t x, int32_t y, int32_t r,
                          uint32_t fg_color, uint32_t bg_color);
    void drawWideLine(float ax, float ay, float bx, float by,
                      float wd, uint32_t color,
                      uint32_t bg_color = TFT_BLACK);

private:
    uint8_t  _rot     = 0;
    uint8_t  _datum   = TL_DATUM;
    int16_t  _padding = 0;
    uint16_t _fg      = TFT_WHITE;
    uint16_t _bg      = TFT_BLACK;

    void    _selectFont(uint8_t font);
    int16_t _fontHeight(uint8_t font);
    int16_t _fontCharWidth(uint8_t font);
};

// DISPLAY_SDO (gp11, SPI1 TX) → mosi; DISPLAY_SDI (gp8, SPI1 RX) → miso
#define DISPLAY_DECLARE_TFT TT_Display tft(DISPLAY_CS, DISPLAY_RST, DISPLAY_SDO, DISPLAY_SCK, DISPLAY_SDI)

#else
  // Display driver not active for this build — either DISPLAY_DRIVER_RA8876_NATIVE
  // is not defined, or the active board header does not define DISPLAY_CS (i.e.
  // no display pin assignment has been made for this board).
  // ClockDisplay.cpp provides no-op stubs for setupDisplay() / drawFastClock().
#endif
