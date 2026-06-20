/*
 *  DisplayDriver.cpp — TT_Display wrapper for LCC RPi Pico Clock_Lights
 *
 *  Adapted from the Turntable project DisplayDriver.cpp.
 *  Turntable-specific geometry (TT_CX/CY/RAD, track drawing) removed.
 */

#include "DisplayDriver.h"
#include "BoardSettings.h"

#if defined(DISPLAY_DRIVER_RA8876_NATIVE) && defined(DISPLAY_CS)

#include <math.h>

// ===========================================================================
//  Font metric tables
//
//  RA8876 internal CGROM fonts (fixed-width):
//    8×16  → RA8876_CHAR_HEIGHT_16 (size_select = 0)
//   12×24  → RA8876_CHAR_HEIGHT_24 (size_select = 1)
//   16×32  → RA8876_CHAR_HEIGHT_32 (size_select = 2)
//
//  Font number → RA8876 mapping:
//    font 1/2  →  8×16,  1× scale  =   8× 16 px
//    font 4    → 12×24,  1× scale  =  12× 24 px
//    font 5    → 16×32,  1× scale  =  16× 32 px
//    font 6/7/8→ 16×32,  2× scale  =  32× 64 px
//    font 10   → 16×32,  4× scale  =  64×128 px  (largest hardware font)
// ===========================================================================

int16_t TT_Display::_fontCharWidth(uint8_t font) {
    switch (font) {
        case 1:
        case 2:  return  8;
        case 4:  return 12;
        case 5:  return 16;
        case 6:
        case 7:
        case 8:  return 32;
        case 10: return 64;   // 16×32 CGROM × 4× scale
        default: return  8;
    }
}

int16_t TT_Display::_fontHeight(uint8_t font) {
    switch (font) {
        case 1:
        case 2:  return  16;
        case 4:  return  24;
        case 5:  return  32;
        case 6:
        case 7:
        case 8:  return  64;
        case 10: return 128;  // 16×32 CGROM × 4× scale
        default: return  16;
    }
}

void TT_Display::_selectFont(uint8_t font) {
    switch (font) {
        case 1:
        case 2:
            setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,
                              RA8876_CHAR_HEIGHT_16,
                              RA8876_SELECT_8859_1);
            setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE,
                              RA8876_TEXT_CHROMA_KEY_DISABLE,
                              RA8876_TEXT_WIDTH_ENLARGEMENT_X1,
                              RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
            _FNTwidth  =  8;
            _FNTheight = 16;
            break;
        case 4:
            setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,
                              RA8876_CHAR_HEIGHT_24,
                              RA8876_SELECT_8859_1);
            setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE,
                              RA8876_TEXT_CHROMA_KEY_DISABLE,
                              RA8876_TEXT_WIDTH_ENLARGEMENT_X1,
                              RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
            _FNTwidth  = 12;
            _FNTheight = 24;
            break;
        case 5:
            setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,
                              RA8876_CHAR_HEIGHT_32,
                              RA8876_SELECT_8859_1);
            setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE,
                              RA8876_TEXT_CHROMA_KEY_DISABLE,
                              RA8876_TEXT_WIDTH_ENLARGEMENT_X1,
                              RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
            _FNTwidth  = 16;
            _FNTheight = 32;
            break;
        case 6:
        case 7:
        case 8:
            setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,
                              RA8876_CHAR_HEIGHT_32,
                              RA8876_SELECT_8859_1);
            setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE,
                              RA8876_TEXT_CHROMA_KEY_DISABLE,
                              RA8876_TEXT_WIDTH_ENLARGEMENT_X2,
                              RA8876_TEXT_HEIGHT_ENLARGEMENT_X2);
            _FNTwidth  = 32;
            _FNTheight = 64;
            break;
        case 10:
            setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,
                              RA8876_CHAR_HEIGHT_32,
                              RA8876_SELECT_8859_1);
            setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE,
                              RA8876_TEXT_CHROMA_KEY_DISABLE,
                              RA8876_TEXT_WIDTH_ENLARGEMENT_X4,
                              RA8876_TEXT_HEIGHT_ENLARGEMENT_X4);
            _FNTwidth  = 64;   // 16 × 4× scale
            _FNTheight = 128;  // 32 × 4× scale
            break;
        default:
            setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,
                              RA8876_CHAR_HEIGHT_16,
                              RA8876_SELECT_8859_1);
            setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE,
                              RA8876_TEXT_CHROMA_KEY_DISABLE,
                              RA8876_TEXT_WIDTH_ENLARGEMENT_X1,
                              RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
            _FNTwidth  =  8;
            _FNTheight = 16;
            break;
    }
}

// ===========================================================================
//  setCursor (3-arg, font-selecting overload)
// ===========================================================================
void TT_Display::setCursor(int32_t x, int32_t y, uint8_t font) {
    _selectFont(font);
    RA8876_RP2040::setCursor((int16_t)x, (int16_t)y);
}

// ===========================================================================
//  textWidth
// ===========================================================================
int16_t TT_Display::textWidth(const char* str, uint8_t font) {
    if (!str) return 0;
    return (int16_t)(strlen(str) * (1 + _fontCharWidth(font)));
}

// ===========================================================================
//  drawString
// ===========================================================================
void TT_Display::drawString(const char* str, int32_t x, int32_t y, uint8_t font) {
    if (!str) return;

    int16_t tw = textWidth(str, font);
    int16_t th = _fontHeight(font);
    int16_t ax = (int16_t)x;
    int16_t ay = (int16_t)y;

    switch (_datum) {
        case TC_DATUM: ax -= tw / 2;                break;
        case TR_DATUM: ax -= tw;                    break;
        case ML_DATUM:              ay -= th / 2;   break;
        case MC_DATUM: ax -= tw / 2; ay -= th / 2;  break;
        case MR_DATUM: ax -= tw;     ay -= th / 2;  break;
        case BL_DATUM:              ay -= th;        break;
        case BC_DATUM: ax -= tw / 2; ay -= th;       break;
        case BR_DATUM: ax -= tw;     ay -= th;       break;
        default: break;
    }

    if (_padding > 0) {
        int16_t clearW = (_padding > tw) ? _padding : tw;
        fillRect(ax, ay, clearW, th, _bg);
    }

    _selectFont(font);
    setTextColor(_fg, _bg);
    RA8876_RP2040::setCursor(ax, ay);
    putString(ax, ay, str);
}

// ===========================================================================
//  fillRect — guard against degenerate dimensions that hang the RA8876 GE
// ===========================================================================
void TT_Display::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (w <= 0 || h <= 0) return;
    if (w == 1) {
        check2dBusy();
        for (int16_t i = 0; i < h; i++)
            drawPixel((uint16_t)x, (uint16_t)(y + i), color);
        return;
    }
    RA8876_RP2040::fillRect(x, y, w, h, color);
}

// ===========================================================================
//  fillTriangle — software scanline fill (RA8876 DRAW_TRIANGLE_FILL hangs)
// ===========================================================================
void TT_Display::fillTriangle(int16_t x0, int16_t y0,
                               int16_t x1, int16_t y1,
                               int16_t x2, int16_t y2, uint16_t color) {
    if (y0 > y1) { int16_t t; t=x0; x0=x1; x1=t; t=y0; y0=y1; y1=t; }
    if (y1 > y2) { int16_t t; t=x1; x1=x2; x2=t; t=y1; y1=y2; y2=t; }
    if (y0 > y1) { int16_t t; t=x0; x0=x1; x1=t; t=y0; y0=y1; y1=t; }

    if (y0 == y2) {
        int16_t xa = min(min(x0, x1), x2);
        int16_t xb = max(max(x0, x1), x2);
        int16_t w  = xb - xa + 1;
        if (w >= 2) fillRect(xa, y0, w, 1, color);
        return;
    }

    int16_t dy02 = y2 - y0;
    int16_t dy01 = y1 - y0;
    int16_t dy12 = y2 - y1;

    for (int16_t y = y0; y <= y1; y++) {
        int16_t xa = x0 + (int16_t)((int32_t)(x2 - x0) * (y - y0) / dy02);
        int16_t xb = (dy01 > 0) ? x0 + (int16_t)((int32_t)(x1 - x0) * (y - y0) / dy01) : x1;
        if (xa > xb) { int16_t t = xa; xa = xb; xb = t; }
        int16_t w = xb - xa + 1;
        if (w >= 2) fillRect(xa, y, w, 1, color);
    }
    for (int16_t y = y1 + 1; y <= y2; y++) {
        int16_t xa = x0 + (int16_t)((int32_t)(x2 - x0) * (y - y0) / dy02);
        int16_t xb = (dy12 > 0) ? x1 + (int16_t)((int32_t)(x2 - x1) * (y - y1) / dy12) : x2;
        if (xa > xb) { int16_t t = xa; xa = xb; xb = t; }
        int16_t w = xb - xa + 1;
        if (w >= 2) fillRect(xa, y, w, 1, color);
    }
}

// ===========================================================================
//  fillCircle — software scanline fill (RA8876 circle GE hangs)
// ===========================================================================
void TT_Display::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    if (r <= 0) {
        check2dBusy();
        drawPixel((uint16_t)x0, (uint16_t)y0, color);
        return;
    }
    int32_t r2 = (int32_t)r * r;
    for (int16_t dy = -r; dy <= r; dy++) {
        int16_t dx = (int16_t)sqrtf((float)(r2 - (int32_t)dy * dy));
        if (dx == 0) continue;
        fillRect(x0 - dx, y0 + dy, (int16_t)(2 * dx + 1), 1, color);
    }
}

// ===========================================================================
//  drawCircle — Bresenham outline via drawPixel (circle GE hangs)
// ===========================================================================
void TT_Display::drawCircle(uint16_t ux0, uint16_t uy0, uint16_t ur, uint16_t color) {
    check2dBusy();
    int16_t x0 = (int16_t)ux0, y0 = (int16_t)uy0, r = (int16_t)ur;
    if (r <= 0) { drawPixel((uint16_t)x0, (uint16_t)y0, color); return; }
    int16_t f = 1 - r, ddF_x = 0, ddF_y = -2 * r, x = 0, y = r;
    drawPixel((uint16_t)x0,       (uint16_t)(y0 + r), color);
    drawPixel((uint16_t)x0,       (uint16_t)(y0 - r), color);
    drawPixel((uint16_t)(x0 + r), (uint16_t)y0,       color);
    drawPixel((uint16_t)(x0 - r), (uint16_t)y0,       color);
    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x + 1;
        drawPixel((uint16_t)(x0+x),(uint16_t)(y0+y),color);
        drawPixel((uint16_t)(x0-x),(uint16_t)(y0+y),color);
        drawPixel((uint16_t)(x0+x),(uint16_t)(y0-y),color);
        drawPixel((uint16_t)(x0-x),(uint16_t)(y0-y),color);
        drawPixel((uint16_t)(x0+y),(uint16_t)(y0+x),color);
        drawPixel((uint16_t)(x0-y),(uint16_t)(y0+x),color);
        drawPixel((uint16_t)(x0+y),(uint16_t)(y0-x),color);
        drawPixel((uint16_t)(x0-y),(uint16_t)(y0-x),color);
    }
}

// ===========================================================================
//  drawSmoothRoundRect — filled border ring via fillRect + fillCircle
// ===========================================================================
static void _fillRoundedRect(TT_Display& d,
                              int32_t x, int32_t y, int32_t w, int32_t h,
                              int32_t r, uint16_t color) {
    if (w <= 0 || h <= 0) return;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (r < 1)     r = 0;
    if (r == 0) { d.fillRect((int16_t)x,(int16_t)y,(int16_t)w,(int16_t)h,color); return; }
    d.fillRect((int16_t)x,       (int16_t)(y+r),   (int16_t)w,     (int16_t)(h-2*r), color);
    d.fillRect((int16_t)(x+r),   (int16_t)y,       (int16_t)(w-2*r),(int16_t)r,      color);
    d.fillRect((int16_t)(x+r),   (int16_t)(y+h-r), (int16_t)(w-2*r),(int16_t)r,      color);
    int16_t cx_l=(int16_t)(x+r), cx_r=(int16_t)(x+w-r-1);
    int16_t cy_t=(int16_t)(y+r), cy_b=(int16_t)(y+h-r-1);
    int16_t rv=(int16_t)r;
    d.fillCircle(cx_l,cy_t,rv,color); d.fillCircle(cx_r,cy_t,rv,color);
    d.fillCircle(cx_l,cy_b,rv,color); d.fillCircle(cx_r,cy_b,rv,color);
}

void TT_Display::drawSmoothRoundRect(int32_t x, int32_t y,
                                     int32_t r, int32_t ir,
                                     int32_t w, int32_t h,
                                     uint32_t fg_color, uint32_t bg_color,
                                     uint8_t /*quadrants*/) {
    _fillRoundedRect(*this, x, y, w, h, r, (uint16_t)fg_color);
    int32_t thickness = r - ir + 1;
    int32_t iw = w - 2 * thickness, ih = h - 2 * thickness;
    if (iw > 0 && ih > 0)
        _fillRoundedRect(*this, x+thickness, y+thickness, iw, ih, ir, (uint16_t)bg_color);
}

void TT_Display::drawSmoothCircle(int32_t x, int32_t y, int32_t r,
                                   uint32_t fg_color, uint32_t /*bg_color*/) {
    drawCircle((int16_t)x, (int16_t)y, (int16_t)r, (uint16_t)fg_color);
}

// ===========================================================================
//  drawWideLine — parallelogram body + circular end caps
// ===========================================================================
void TT_Display::drawWideLine(float ax, float ay, float bx, float by,
                               float wd, uint32_t color, uint32_t /*bg_color*/) {
    float dx = bx - ax, dy = by - ay;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 0.5f) {
        fillCircle((int16_t)(ax+0.5f),(int16_t)(ay+0.5f),(int16_t)(wd*0.5f+0.5f),(uint16_t)color);
        return;
    }
    float hw = wd * 0.5f, px = -dy/len*hw, py = dx/len*hw;
    int16_t x0=(int16_t)(ax+px+0.5f), y0=(int16_t)(ay+py+0.5f);
    int16_t x1=(int16_t)(ax-px+0.5f), y1=(int16_t)(ay-py+0.5f);
    int16_t x2=(int16_t)(bx-px+0.5f), y2=(int16_t)(by-py+0.5f);
    int16_t x3=(int16_t)(bx+px+0.5f), y3=(int16_t)(by+py+0.5f);
    fillTriangle(x0,y0,x1,y1,x2,y2,(uint16_t)color);
    fillTriangle(x0,y0,x2,y2,x3,y3,(uint16_t)color);
    int16_t capR=(int16_t)(hw+0.5f);
    fillCircle((int16_t)(ax+0.5f),(int16_t)(ay+0.5f),capR,(uint16_t)color);
    fillCircle((int16_t)(bx+0.5f),(int16_t)(by+0.5f),capR,(uint16_t)color);
}

// ===========================================================================
//  drawRGB565 — bulk-blit a packed RGB565 pixel array into a rectangle.
//
//  Follows the same SPI transaction pattern as bteMpuWriteWithChromaKeyData8:
//    1. putPicture_16bpp() sets the canvas active window + cursor and issues
//       the RAM-access-prepare command (ends its own SPI transaction).
//    2. A new startSend() transaction sends RA8876_SPI_DATAWRITE then the
//       raw pixel bytes, flushed in 4 KB chunks through a static RAM buffer
//       so DMA reads are safe even when `data` lives in XIP flash (PROGMEM).
// ===========================================================================
void TT_Display::drawRGB565(int16_t x, int16_t y, int16_t w, int16_t h,
                             const uint16_t* data) {
    if (w <= 0 || h <= 0 || !data) return;

    // Canvas window + RAM-write-prepare (ends its own SPI transaction)
    putPicture_16bpp((ru16)x, (ru16)y, (ru16)w, (ru16)h);

    // Bulk pixel transfer — copied through a RAM buffer (XIP/PROGMEM-safe)
    static uint8_t _dma[4096];
    const uint8_t* src = (const uint8_t*)data;
    uint32_t rem = (uint32_t)w * h * 2;

    startSend();
    _pspi->transfer((uint8_t)RA8876_SPI_DATAWRITE);
    while (rem > 0) {
        uint32_t n = (rem < sizeof(_dma)) ? rem : sizeof(_dma);
        memcpy(_dma, src, n);
        _pspi->transfer(_dma, nullptr, n);
        src += n;
        rem -= n;
    }
    endSend(true);

    // Restore full-screen active window
    activeWindowXY(0, 0);
    activeWindowWH(_width, _height);
}

// ===========================================================================
//  drawBitmask1bit — blit a 1-bit monochrome image with a two-tone background.
//
//  bits:  MSB-first packed data, ceil(w/8) bytes per row, row-major.
//  Pixel=1 → fg_color (white digit).
//  Pixel=0 → bg_top for rows < split_y, bg_bot for rows >= split_y.
//  A 4-pixel band at split_y..split_y+3 is rendered solid split_color
//  regardless of the image data (simulates the split-flap divider line).
//
//  Uses a per-scanline RGB565 buffer so the canvas window is opened once
//  and pixel data is streamed continuously — fast at 20 MHz SPI.
// ===========================================================================
void TT_Display::drawBitmask1bit(int16_t x,  int16_t y,
                                  int16_t w,  int16_t h,
                                  const uint8_t* bits,
                                  uint16_t fg,
                                  uint16_t bg_top, uint16_t bg_bot,
                                  int16_t split_y, uint16_t split_color) {
    if (w <= 0 || h <= 0 || !bits) return;

    putPicture_16bpp((ru16)x, (ru16)y, (ru16)w, (ru16)h);

    // Buffer must fit the widest tile (PRAG_IMG_HOUR_W = 367px → 734 bytes).
    // 800 bytes covers up to 400px with margin.
    static uint8_t _row[800];
    const int rowBytes = (w + 7) / 8;

    startSend();
    _pspi->transfer((uint8_t)RA8876_SPI_DATAWRITE);

    for (int16_t row = 0; row < h; row++) {
        const uint8_t* src   = bits + row * rowBytes;
        const bool     isSpl = (row >= split_y && row < split_y + 4);
        const uint16_t bg    = (row < split_y) ? bg_top : bg_bot;
        uint8_t* out = _row;

        for (int16_t col = 0; col < w; col++) {
            uint16_t c;
            if (isSpl) {
                c = split_color;
            } else {
                bool bit = (src[col >> 3] >> (7 - (col & 7))) & 1;
                c = bit ? fg : bg;
            }
            // Low byte first — matches the little-endian memory layout that
            // _pspi->transfer(buf,nullptr,n) produces from uint16_t arrays,
            // consistent with drawRGB565 / the Nixie image path.
            *out++ = (uint8_t)(c & 0xFF);
            *out++ = (uint8_t)(c >> 8);
        }
        _pspi->transfer(_row, nullptr, (uint32_t)w * 2);
    }

    endSend(true);
    activeWindowXY(0, 0);
    activeWindowWH(_width, _height);
}

#endif  // DISPLAY_DRIVER_RA8876_NATIVE && DISPLAY_CS
