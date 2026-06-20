/*
 *  ClockDisplay.cpp — Phase 2: analog + digital styles, button panel
 *
 *  STYLE_ANALOG (default)
 *    Analog clock face, radius 215 px, centred at (320, 240) within the
 *    left 640 px of the 800×480 screen.  Four buttons occupy a 158 px
 *    column on the right.
 *
 *  STYLE_DIGITAL
 *    Large 7-segment HH:MM (Phase 1 renderer, unchanged).  Four buttons
 *    occupy an 80 px bar across the bottom.
 *
 *  Button panel (both styles)
 *    [0] BTN A     — configurable LCC producer event
 *    [1] BTN B     — configurable LCC producer event
 *    [2] RUN/PAUSE — green when running, red when stopped
 *    [3] STYLE     — cycle clock face  (always bottom-right)
 *
 *  Touch support is stubbed out (processTouchDisplay is a no-op).
 *  Wired up in Phase 3 once the XPT2046 breakout is available.
 */

#include "ClockDisplay.h"
#include "DisplayDriver.h"
#include "BoardSettings.h"
#include <math.h>
#include <SPI.h>
#include <EEPROM.h>   // RP2040 internal flash EEPROM — separate from external I2C EEPROM

static const int STYLE_EEPROM_ADDR  = 0;  // address within RP2040 EEPROM emulation
static const int HOUR12_EEPROM_ADDR = 1;  // 0 = 24h, 1 = 12h

// Use JMRI Nixie image assets if the generated header is present.
#if __has_include("nixie_images.h")
#  include "nixie_images.h"
#  define NIXIE_USE_IMAGES 1
#else
#  define NIXIE_USE_IMAGES 0
#endif

#if __has_include("pragotron_images.h")
#  include "pragotron_images.h"
#  define PRAG_USE_IMAGES 1
#else
#  define PRAG_USE_IMAGES 0
#endif

#ifdef TOUCH_CS
#pragma message "ClockDisplay: TOUCH_CS defined — touch enabled"
#else
#warning "ClockDisplay: TOUCH_CS not defined — touch disabled"
#endif

#if defined(DISPLAY_DRIVER_RA8876_NATIVE) && defined(DISPLAY_CS)

DISPLAY_DECLARE_TFT;

// ===========================================================================
//  Colour palette
// ===========================================================================
static const uint16_t C_BG          = TFT_BLACK;

// Analog face
static const uint16_t C_FACE_BG     = 0x0820;   // very dark navy
static const uint16_t C_RING        = TFT_WHITE;
static const uint16_t C_TICK_H      = TFT_WHITE; // hour tick marks
static const uint16_t C_TICK_M      = 0x8C71;   // minute tick marks  (grey)
static const uint16_t C_NUM         = TFT_WHITE; // hour numerals
static const uint16_t C_HAND        = 0xFD20;   // amber — matches digital style
static const uint16_t C_CENTER_DOT  = 0xF800;   // red centre pin

// Digital face (7-segment)
static const uint16_t C_SEG_ON      = 0xFD20;   // active segment   (amber)
static const uint16_t C_SEG_DIM     = 0x18C3;   // inactive ghost

// Nixie tube face  (warm amber glow on dark amber tube backgrounds)
static const uint16_t C_NIXIE_BG       = 0x0000;  // screen between tubes — black
static const uint16_t C_NIXIE_TUBE     = 0x5940;  // tube fill    ~#5A2800
static const uint16_t C_NIXIE_TUBE_RIM = 0x7180;  // tube border  ~#703000
static const uint16_t C_NIXIE_GLOW0    = 0xA200;  // outer glow   ~#A04000
static const uint16_t C_NIXIE_GLOW1    = 0xFB00;  // mid glow     ~#FF6000
static const uint16_t C_NIXIE_CORE     = 0xFEC0;  // bright core  ~#FFD800

// Pragotron (split-flap) face  (white printed numerals on dark split tiles)
static const uint16_t C_PRAG_BG        = 0x2104;  // screen background  ~#212121
static const uint16_t C_PRAG_TILE_TOP  = 0x3186;  // tile top half      ~#303030
static const uint16_t C_PRAG_TILE_BOT  = 0x2945;  // tile bottom half   ~#282828
static const uint16_t C_PRAG_SPLIT     = 0x0000;  // split line — black
static const uint16_t C_PRAG_BORDER    = 0x4208;  // tile border        ~#404040
static const uint16_t C_PRAG_ON        = 0xFFFF;  // digit — white

// Buttons
static const uint16_t C_BTN_BG      = 0x2124;   // dark grey fill
static const uint16_t C_BTN_BORDER  = TFT_WHITE;
static const uint16_t C_BTN_TXT     = TFT_WHITE;
static const uint16_t C_BTN_RUN     = 0x07E0;   // green  — clock running
static const uint16_t C_BTN_STOP    = 0xF800;   // red    — clock stopped
static const uint16_t C_DIVIDER     = 0x4208;   // dim separator line

// ===========================================================================
//  State
// ===========================================================================
static ClockStyle _style   = STYLE_ANALOG;  // default; persisted EEPROM in Phase 3
static bool       _running = true;          // last-known clock running state
static int        _lastHour   = 0;          // last time received from LCC
static int        _lastMinute = 0;
static bool       _hour12  = false;         // false=24h, true=12h; persisted EEPROM

// ===========================================================================
//  Button geometry
// ===========================================================================
struct ButtonRect { int16_t x, y, w, h; };
static ButtonRect _btns[4];

static const char* _btnLabel[4] = { "BTN A", "BTN B", "---", "STYLE" };

static void _computeButtons() {
    if (_style == STYLE_ANALOG) {
        // Vertical column on the right: x 642..797, four rows of 120 px
        for (int i = 0; i < 4; i++)
            _btns[i] = { 642, (int16_t)(i * 120 + 1), 155, 118 };
    } else {
        // Horizontal bar on the bottom: y 402..478, four equal columns
        // (DIGITAL, NIXIE, and PRAGOTRON all use this layout)
        for (int i = 0; i < 4; i++)
            _btns[i] = { (int16_t)(i * 200 + 1), 402, 198, 76 };
    }
}

// ---------------------------------------------------------------------------
//  Draw one button (redrawn on state change as well as style change)
// ---------------------------------------------------------------------------
static void _drawOneButton(uint8_t idx) {
    const ButtonRect& b = _btns[idx];

    // Background — Run/Pause uses state colour, others use neutral dark grey
    uint16_t bg = C_BTN_BG;
    if (idx == 2) bg = _running ? C_BTN_RUN : C_BTN_STOP;  // RUN/PAUSE is button 2
    tft.fillRect(b.x, b.y, b.w, b.h, bg);

    // One-pixel border
    tft.drawLine(b.x,           b.y,           b.x + b.w - 1, b.y,           C_BTN_BORDER);
    tft.drawLine(b.x,           b.y + b.h - 1, b.x + b.w - 1, b.y + b.h - 1, C_BTN_BORDER);
    tft.drawLine(b.x,           b.y,           b.x,            b.y + b.h - 1, C_BTN_BORDER);
    tft.drawLine(b.x + b.w - 1, b.y,           b.x + b.w - 1, b.y + b.h - 1, C_BTN_BORDER);

    // Label centred in button
    const char* label = _btnLabel[idx];
    if (idx == 2) label = _running ? "PAUSE" : "RESUME";  // dynamic Run/Pause label

    tft.setTextColor(C_BTN_TXT, bg);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(label, b.x + b.w / 2, b.y + b.h / 2, 2);
    tft.setTextDatum(TL_DATUM);
}

static void _drawButtons() {
    // Separator line between clock area and button panel
    if (_style == STYLE_ANALOG) {
        tft.drawLine(640, 0,   640, VRES, C_DIVIDER);
        tft.drawLine(641, 0,   641, VRES, C_DIVIDER);
    } else {
        tft.drawLine(0,   400, HRES, 400, C_DIVIDER);
        tft.drawLine(0,   401, HRES, 401, C_DIVIDER);
    }
    for (int i = 0; i < 4; i++) _drawOneButton(i);
}

// ===========================================================================
//  7-segment digital renderer  (Phase 1, unchanged)
// ===========================================================================

//  Bit order: a=0 b=1 c=2 d=3 e=4 f=5 g=6
//       aaa
//      f   b
//       ggg
//      e   c
//       ddd
static const uint8_t _seg7[10] = {
    0b0111111,  // 0
    0b0000110,  // 1
    0b1011011,  // 2
    0b1001111,  // 3
    0b1100110,  // 4
    0b1101101,  // 5
    0b1111101,  // 6
    0b0000111,  // 7
    0b1111111,  // 8
    0b1101111,  // 9
};

static const int16_t DIG_W     = 150;
static const int16_t DIG_H     = 300;
static const int16_t DIG_T     =  18;
static const int16_t DIG_G     =  10;  // gap: digit→colon and colon→digit
static const int16_t DIG_G_PAIR=  28;  // wider gap between tens and ones within each group
static const int16_t COL_W     =  46;
static const int16_t COL_D  =  22;
static const int16_t DIG_Y  =  50;  // top of digit row (leaves 350 px, well above button bar)

static int16_t _xH1, _xH2, _xCol, _xM1, _xM2;
static int8_t  _prev[4] = { -1, -1, -1, -1 };

static inline void _segFill(int16_t x, int16_t y, int16_t w, int16_t h, bool on) {
    tft.fillRect(x, y, w, h, on ? C_SEG_ON : C_SEG_DIM);
}

static void _drawDigit(int16_t x, int16_t y, uint8_t d) {
    const uint8_t  s  = (d <= 9) ? _seg7[d] : 0;
    const int16_t  T  = DIG_T, W = DIG_W, H = DIG_H;
    const int16_t  hs = W - 2*T, vs = H/2 - 2*T;
    _segFill(x + T,     y,              hs, T,  s & 0x01);  // a — top
    _segFill(x + W - T, y + T,          T,  vs, s & 0x02);  // b — top-right
    _segFill(x + W - T, y + H/2 + T,   T,  vs, s & 0x04);  // c — bot-right
    _segFill(x + T,     y + H - T,      hs, T,  s & 0x08);  // d — bottom
    _segFill(x,         y + H/2 + T,   T,  vs, s & 0x10);  // e — bot-left
    _segFill(x,         y + T,          T,  vs, s & 0x20);  // f — top-left
    _segFill(x + T,     y + H/2 - T/2, hs, T,  s & 0x40);  // g — middle
}

static void _drawColon(int16_t x, int16_t y) {
    const int16_t cx = x + (COL_W - COL_D) / 2;
    tft.fillRect(cx, y + DIG_H / 3     - COL_D / 2, COL_D, COL_D, C_SEG_ON);
    tft.fillRect(cx, y + 2 * DIG_H / 3 - COL_D / 2, COL_D, COL_D, C_SEG_ON);
}

// Shared layout calculation for all digit-based styles (digital / nixie / pragotron).
static void _computeDigitLayout() {
    // H1→H2 and M1→M2 use DIG_G_PAIR (wider); colon margins use DIG_G (tighter)
    const int16_t totalW = 4*DIG_W + 2*DIG_G_PAIR + 2*DIG_G + COL_W;
    const int16_t leftX  = (HRES - totalW) / 2;
    _xH1  = leftX;
    _xH2  = _xH1  + DIG_W + DIG_G_PAIR;  // wider within-hour gap
    _xCol = _xH2  + DIG_W + DIG_G;
    _xM1  = _xCol + COL_W + DIG_G;
    _xM2  = _xM1  + DIG_W + DIG_G_PAIR;  // wider within-minute gap
}

static void _setupDigital() {
    _computeDigitLayout();
    _drawDigit(_xH1, DIG_Y, 10); _drawDigit(_xH2, DIG_Y, 10);
    _drawDigit(_xM1, DIG_Y, 10); _drawDigit(_xM2, DIG_Y, 10);
    _drawColon(_xCol, DIG_Y);
    for (int i = 0; i < 4; i++) _prev[i] = -1;
}


static void _updateDigital(int hour, int minute) {
    const int8_t  d[4] = { (int8_t)(hour/10),   (int8_t)(hour%10),
                            (int8_t)(minute/10), (int8_t)(minute%10) };
    const int16_t x[4] = { _xH1, _xH2, _xM1, _xM2 };
    for (int i = 0; i < 4; i++) {
        if (d[i] != _prev[i]) {
            _drawDigit(x[i], DIG_Y, (uint8_t)d[i]);
            _prev[i] = d[i];
        }
    }
}

// ===========================================================================
//  Proportional stroke font — shared by Nixie and Pragotron
//
//  Each digit is defined as up to 5 line segments (strokes) in a
//  PF_W × PF_H coordinate space.  drawWideLine gives natural rounded ends.
//  Stroke endpoints are offset PF_EC from cell edges so the end-caps fit.
//
//  PF_W = 120, PF_H = 200, PF_SW = 22 (stroke width), PF_EC = 11 (end-cap)
//  Corner positions in the grid:
//    left  x = PF_EC = 11      right  x = PF_W - PF_EC = 109
//    top   y = PF_EC = 11      mid    y = PF_H/2        = 100
//    bot   y = PF_H - PF_EC  = 189
// ===========================================================================
static const int16_t PF_W  = 120;   // digit cell width  (pixels)
static const int16_t PF_H  = 200;   // digit cell height (pixels)
static const float   PF_SW = 22.0f; // stroke width
static const int16_t PF_EC = 11;    // end-cap offset  (= PF_SW/2 rounded up)
static const int16_t PF_GAP= 8;     // gap between tiles
static const int16_t PF_SEP= 26;    // separator tile width

struct PFStroke { int16_t x1, y1, x2, y2; };

// Stroke counts and definitions — 5 slots per digit, unused slots ignored.
static const uint8_t _pfN[10] = { 4, 2, 5, 4, 3, 5, 5, 2, 5, 5 };
static const PFStroke _pfS[10][5] = {
    // 0: top, right, bottom, left
    {{11,11,109,11},{109,11,109,189},{11,189,109,189},{11,11,11,189},{0,0,0,0}},
    // 1: top-left serif → bar top, then vertical bar
    {{50,35,95,11},{95,11,95,189},{0,0,0,0},{0,0,0,0},{0,0,0,0}},
    // 2: top, top-right, mid, bot-left, bottom
    {{11,11,109,11},{109,11,109,100},{11,100,109,100},{11,100,11,189},{11,189,109,189}},
    // 3: top, right, mid, bottom
    {{11,11,109,11},{109,11,109,189},{11,100,109,100},{11,189,109,189},{0,0,0,0}},
    // 4: top-left, mid, right
    {{11,11,11,100},{11,100,109,100},{109,11,109,189},{0,0,0,0},{0,0,0,0}},
    // 5: top, top-left, mid, bot-right, bottom
    {{11,11,109,11},{11,11,11,100},{11,100,109,100},{109,100,109,189},{11,189,109,189}},
    // 6: top, left, mid, bot-right, bottom
    {{11,11,109,11},{11,11,11,189},{11,100,109,100},{109,100,109,189},{11,189,109,189}},
    // 7: top, slight-slant right
    {{11,11,109,11},{109,11,80,189},{0,0,0,0},{0,0,0,0},{0,0,0,0}},
    // 8: top, mid, bottom, left, right
    {{11,11,109,11},{11,100,109,100},{11,189,109,189},{11,11,11,189},{109,11,109,189}},
    // 9: top, top-left, mid, right, bottom
    {{11,11,109,11},{11,11,11,100},{11,100,109,100},{109,11,109,189},{11,189,109,189}},
};

// Shared layout positions (set by _computePFLayout, used by both styles).
static int16_t _pfH1, _pfH2, _pfSep, _pfM1, _pfM2, _pfY;
static int16_t _pfTW, _pfTH;  // tile width / height for current style

static void _computePFLayout(int16_t pad) {
    _pfTW = PF_W + 2 * pad;
    _pfTH = PF_H + 2 * pad;
    const int16_t total = 4 * _pfTW + PF_SEP + 4 * PF_GAP;
    _pfH1  = (HRES - total) / 2;
    _pfH2  = _pfH1  + _pfTW + PF_GAP;
    _pfSep = _pfH2  + _pfTW + PF_GAP;
    _pfM1  = _pfSep + PF_SEP + PF_GAP;
    _pfM2  = _pfM1  + _pfTW + PF_GAP;
    _pfY   = (400 - _pfTH) / 2;   // vertically centred above button bar (y=400)
}

// Draw the strokes for digit d with origin at (ox, oy) in given colour/width.
static void _pfDraw(int16_t ox, int16_t oy, uint8_t d, float sw, uint16_t col) {
    if (d > 9) return;
    for (uint8_t i = 0; i < _pfN[d]; i++)
        tft.drawWideLine(ox + _pfS[d][i].x1, oy + _pfS[d][i].y1,
                         ox + _pfS[d][i].x2, oy + _pfS[d][i].y2,
                         sw, col, col);
}

// ===========================================================================
//  Nixie tube renderer
//
//  When nixie_images.h is present (NIXIE_USE_IMAGES=1): blits the JMRI
//  RGB565 image tiles directly — authentic photographic glow, fastest draw.
//
//  Fallback (NIXIE_USE_IMAGES=0): stroke-based 3-pass glow renderer using
//  the shared proportional font (_pfDraw).
// ===========================================================================

#if NIXIE_USE_IMAGES
// ---------------------------------------------------------------------------
//  Image-based Nixie renderer
// ---------------------------------------------------------------------------
static const int16_t NX_GAP = 8;  // gap between tubes

// Layout positions — computed once in _setupNixie()
static int16_t _nxH1, _nxH2, _nxCol, _nxM1, _nxM2, _nxY;

static void _drawNixieDigit(int16_t tx, int16_t ty, uint8_t d) {
    if (d <= 9)
        tft.drawRGB565(tx, ty, NIXIE_DIGIT_W, NIXIE_DIGIT_H, nixie_digits[d]);
    else
        tft.fillRect(tx, ty, NIXIE_DIGIT_W, NIXIE_DIGIT_H, C_NIXIE_BG);
}

static void _setupNixie() {
    const int16_t total = 4*NIXIE_DIGIT_W + NIXIE_COLON_W + 4*NX_GAP;
    _nxH1  = (HRES - total) / 2;
    _nxH2  = _nxH1  + NIXIE_DIGIT_W + NX_GAP;
    _nxCol = _nxH2  + NIXIE_DIGIT_W + NX_GAP;
    _nxM1  = _nxCol + NIXIE_COLON_W + NX_GAP;
    _nxM2  = _nxM1  + NIXIE_DIGIT_W + NX_GAP;
    _nxY   = (400 - NIXIE_DIGIT_H) / 2;

    tft.fillScreen(C_NIXIE_BG);
    tft.drawRGB565(_nxCol, _nxY, NIXIE_COLON_W, NIXIE_COLON_H, nixie_colon);
    _drawNixieDigit(_nxH1, _nxY, 10);  // blank tubes on first draw
    _drawNixieDigit(_nxH2, _nxY, 10);
    _drawNixieDigit(_nxM1, _nxY, 10);
    _drawNixieDigit(_nxM2, _nxY, 10);
    for (int i = 0; i < 4; i++) _prev[i] = -1;
}

static void _updateNixie(int hour, int minute) {
    const int8_t  d[4] = { (int8_t)(hour/10),   (int8_t)(hour%10),
                            (int8_t)(minute/10), (int8_t)(minute%10) };
    const int16_t x[4] = { _nxH1, _nxH2, _nxM1, _nxM2 };
    for (int i = 0; i < 4; i++) {
        if (d[i] != _prev[i]) {
            _drawNixieDigit(x[i], _nxY, (uint8_t)d[i]);
            _prev[i] = d[i];
        }
    }
}

#else
// ---------------------------------------------------------------------------
//  Stroke-based fallback Nixie renderer (no image header available)
// ---------------------------------------------------------------------------
static void _drawNixieTube(int16_t tx, int16_t ty) {
    tft.fillRect(tx, ty, _pfTW, _pfTH, C_NIXIE_TUBE);
    tft.drawRect(tx, ty, _pfTW, _pfTH, C_NIXIE_TUBE_RIM);
}

static void _drawNixieDigit(int16_t tx, int16_t ty, uint8_t d) {
    _drawNixieTube(tx, ty);
    if (d <= 9) {
        const int16_t ox = tx + (_pfTW - PF_W) / 2;
        const int16_t oy = ty + (_pfTH - PF_H) / 2;
        _pfDraw(ox, oy, d, PF_SW + 14.0f, C_NIXIE_GLOW0);
        _pfDraw(ox, oy, d, PF_SW +  6.0f, C_NIXIE_GLOW1);
        _pfDraw(ox, oy, d, PF_SW,         C_NIXIE_CORE);
    }
}

static void _drawNixieSep(int16_t tx, int16_t ty) {
    tft.fillRect(tx, ty, PF_SEP, _pfTH, C_NIXIE_TUBE);
    const int16_t cx = tx + PF_SEP / 2, r = 7;
    const int16_t y1 = ty + _pfTH / 3, y2 = ty + 2 * _pfTH / 3;
    tft.fillRect(cx-r-3, y1-r-3, (r+3)*2, (r+3)*2, C_NIXIE_GLOW0);
    tft.fillRect(cx-r-3, y2-r-3, (r+3)*2, (r+3)*2, C_NIXIE_GLOW0);
    tft.fillRect(cx-r-1, y1-r-1, (r+1)*2, (r+1)*2, C_NIXIE_GLOW1);
    tft.fillRect(cx-r-1, y2-r-1, (r+1)*2, (r+1)*2, C_NIXIE_GLOW1);
    tft.fillCircle(cx, y1, r, C_NIXIE_CORE);
    tft.fillCircle(cx, y2, r, C_NIXIE_CORE);
}

static void _setupNixie() {
    _computePFLayout(10);
    tft.fillScreen(C_NIXIE_BG);
    _drawNixieDigit(_pfH1, _pfY, 10); _drawNixieDigit(_pfH2, _pfY, 10);
    _drawNixieDigit(_pfM1, _pfY, 10); _drawNixieDigit(_pfM2, _pfY, 10);
    _drawNixieSep(_pfSep, _pfY);
    for (int i = 0; i < 4; i++) _prev[i] = -1;
}

static void _updateNixie(int hour, int minute) {
    const int8_t  d[4] = { (int8_t)(hour/10),   (int8_t)(hour%10),
                            (int8_t)(minute/10), (int8_t)(minute%10) };
    const int16_t x[4] = { _pfH1, _pfH2, _pfM1, _pfM2 };
    for (int i = 0; i < 4; i++) {
        if (d[i] != _prev[i]) {
            _drawNixieDigit(x[i], _pfY, (uint8_t)d[i]);
            _prev[i] = d[i];
        }
    }
}
#endif  // NIXIE_USE_IMAGES

// ===========================================================================
//  Stroke-based flip clock renderer (STYLE_FLIP) — always compiled.
//  White proportional stroke digits on dark split tiles.
//  Also used as the fallback when pragotron_images.h is not present.
// ===========================================================================
static void _flipTile(int16_t tx, int16_t ty) {
    tft.fillRect(tx, ty,              _pfTW, _pfTH / 2, C_PRAG_TILE_TOP);
    tft.fillRect(tx, ty + _pfTH / 2, _pfTW, _pfTH / 2, C_PRAG_TILE_BOT);
    tft.drawRect(tx, ty, _pfTW, _pfTH, C_PRAG_BORDER);
}

static void _flipDigit(int16_t tx, int16_t ty, uint8_t d) {
    _flipTile(tx, ty);
    if (d <= 9) _pfDraw(tx + (_pfTW - PF_W) / 2, ty + (_pfTH - PF_H) / 2,
                        d, PF_SW, C_PRAG_ON);
    tft.fillRect(tx, ty + _pfTH / 2 - 2, _pfTW, 4, C_PRAG_SPLIT);
}

static void _flipSep(int16_t tx, int16_t ty) {
    tft.fillRect(tx, ty,              PF_SEP, _pfTH / 2, C_PRAG_TILE_TOP);
    tft.fillRect(tx, ty + _pfTH / 2, PF_SEP, _pfTH / 2, C_PRAG_TILE_BOT);
    tft.drawRect(tx, ty, PF_SEP, _pfTH, C_PRAG_BORDER);
    tft.fillCircle(tx + PF_SEP / 2, ty + _pfTH * 3 / 4, 6, C_PRAG_ON);
    tft.fillRect(tx, ty + _pfTH / 2 - 2, PF_SEP, 4, C_PRAG_SPLIT);
}

static void _setupFlip() {
    _computePFLayout(12);
    tft.fillScreen(C_PRAG_BG);
    _flipDigit(_pfH1, _pfY, 10); _flipDigit(_pfH2, _pfY, 10);
    _flipDigit(_pfM1, _pfY, 10); _flipDigit(_pfM2, _pfY, 10);
    _flipSep(_pfSep, _pfY);
    for (int i = 0; i < 4; i++) _prev[i] = -1;
}

static void _updateFlip(int hour, int minute) {
    const int8_t  d[4] = { (int8_t)(hour/10),   (int8_t)(hour%10),
                            (int8_t)(minute/10), (int8_t)(minute%10) };
    const int16_t x[4] = { _pfH1, _pfH2, _pfM1, _pfM2 };
    for (int i = 0; i < 4; i++) {
        if (d[i] != _prev[i]) { _flipDigit(x[i], _pfY, (uint8_t)d[i]); _prev[i] = d[i]; }
    }
}

// ===========================================================================
//  Pragotron (split-flap) renderer
//
//  When pragotron_images.h is present (PRAG_USE_IMAGES=1): blits 1-bit JMRI
//  tiles through drawBitmask1bit — authentic font, fast scanline streaming.
//  Hour: combined H0–H23 tile (367×220).  Minutes: two M0–M9 tiles (174×220).
//
//  Fallback (PRAG_USE_IMAGES=0): stroke-based proportional font renderer.
// ===========================================================================

#if PRAG_USE_IMAGES
// ---------------------------------------------------------------------------
//  Image-based Pragotron renderer
// ---------------------------------------------------------------------------
static const int16_t PG_GAP = 8;   // gap between tiles

// Layout positions — computed in _setupPragotron()
static int16_t _pgHour, _pgDot, _pgMinT, _pgMinO, _pgY;
// Change-tracking (reuse _prev[]: 0=hour, 1=minTens, 2=minOnes)

static void _drawPragHour(int16_t tx, int16_t ty, int h) {
    tft.drawBitmask1bit(tx, ty, PRAG_IMG_HOUR_W, PRAG_IMG_H,
                        prag_hour[h],
                        C_PRAG_ON, C_PRAG_TILE_TOP, C_PRAG_TILE_BOT,
                        PRAG_IMG_H / 2, C_PRAG_SPLIT);
}

static void _drawPragMin(int16_t tx, int16_t ty, int d) {
    tft.drawBitmask1bit(tx, ty, PRAG_IMG_MIN_W, PRAG_IMG_H,
                        prag_min[d],
                        C_PRAG_ON, C_PRAG_TILE_TOP, C_PRAG_TILE_BOT,
                        PRAG_IMG_H / 2, C_PRAG_SPLIT);
}

static void _drawPragDot(int16_t tx, int16_t ty) {
    tft.drawBitmask1bit(tx, ty, PRAG_IMG_DOT_W, PRAG_IMG_H,
                        prag_dot,
                        C_PRAG_ON, C_PRAG_TILE_TOP, C_PRAG_TILE_BOT,
                        PRAG_IMG_H / 2, C_PRAG_SPLIT);
}

static void _setupPragotron() {
    const int16_t total = PRAG_IMG_HOUR_W + PRAG_IMG_DOT_W + 2*PRAG_IMG_MIN_W + 3*PG_GAP;
    _pgHour = (HRES - total) / 2;
    _pgDot  = _pgHour + PRAG_IMG_HOUR_W + PG_GAP;
    _pgMinT = _pgDot  + PRAG_IMG_DOT_W  + PG_GAP;
    _pgMinO = _pgMinT + PRAG_IMG_MIN_W  + PG_GAP;
    _pgY    = (400 - PRAG_IMG_H) / 2;

    tft.fillScreen(C_PRAG_BG);
    _drawPragDot(_pgDot, _pgY);
    // Force full redraw on first updatePragotron call
    _prev[0] = -1;  // hour
    _prev[1] = -1;  // minute tens
    _prev[2] = -1;  // minute ones
}

static void _updatePragotron(int hour, int minute) {
    if ((int8_t)hour         != _prev[0]) { _drawPragHour(_pgHour, _pgY, hour);           _prev[0] = (int8_t)hour; }
    if ((int8_t)(minute / 10) != _prev[1]) { _drawPragMin (_pgMinT, _pgY, minute / 10);   _prev[1] = (int8_t)(minute / 10); }
    if ((int8_t)(minute % 10) != _prev[2]) { _drawPragMin (_pgMinO, _pgY, minute % 10);   _prev[2] = (int8_t)(minute % 10); }
}

// ---------------------------------------------------------------------------
//  Fallback: no images — Pragotron uses the stroke renderer
// ---------------------------------------------------------------------------
#else
static void _setupPragotron()              { _setupFlip(); }
static void _updatePragotron(int h, int m) { _updateFlip(h, m); }
#endif  // PRAG_USE_IMAGES

// ===========================================================================
//  Analog clock renderer
// ===========================================================================

// Face geometry — clock is centred at (320, 240) in the left 640 px zone
static const int16_t FACE_CX = 320;
static const int16_t FACE_CY = 240;
static const int16_t FACE_R  = 215;   // face radius
static const int16_t HOUR_L  = 110;   // hour hand length — kept clear of tick ring (inner edge ~189 px)
static const int16_t MIN_L   = 175;   // minute hand length — may overlap numerals; redrawn at ±1 of each 5-min mark

static const char* _hrNums[12] = {
    "12","1","2","3","4","5","6","7","8","9","10","11"
};

// Previous hand angles — NaN signals "no previous position, do full redraw".
static float _prevHa  = NAN;
static float _prevMa  = NAN;
static int   _prevMin = 0;

static void _eraseHand(float angle, int16_t len, float w) {
    if (!isnan(angle))
        tft.drawWideLine(FACE_CX, FACE_CY,
                         FACE_CX + len * sinf(angle), FACE_CY - len * cosf(angle),
                         w, C_FACE_BG, C_FACE_BG);
}

static void _drawHand(float angle, int16_t len, float w) {
    tft.drawWideLine(FACE_CX, FACE_CY,
                     FACE_CX + len * sinf(angle), FACE_CY - len * cosf(angle),
                     w, C_HAND, C_FACE_BG);
}

static void _moveHand(float prevAngle, float newAngle, int16_t len, float w) {
    _eraseHand(prevAngle, len, w);
    _drawHand(newAngle, len, w);
}

// Numeral font and radius — font 5 = 16×32 px; radius chosen to sit
// inside the tick ring with comfortable clearance.
static const uint8_t  NUM_FONT   = 5;
static const int16_t  NUM_RADIUS = FACE_R - 46;

static void _drawNumeral(int i) {
    const float   a  = i * 30.0f * DEG_TO_RAD;
    const int16_t nx = FACE_CX + (int16_t)(NUM_RADIUS * sinf(a));
    const int16_t ny = FACE_CY - (int16_t)(NUM_RADIUS * cosf(a));
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_NUM, C_FACE_BG);
    tft.drawString(_hrNums[i], nx, ny, NUM_FONT);
    tft.setTextDatum(TL_DATUM);
}

static void _drawFaceStaticElements() {
    // ── Clear interior (preserve the 3 px ring drawn in _setupAnalog) ────
    tft.fillCircle(FACE_CX, FACE_CY, FACE_R - 3, C_FACE_BG);

    // ── 60 tick marks ────────────────────────────────────────────────────
    const int16_t tickOuter = FACE_R - 4;
    for (int i = 0; i < 60; i++) {
        const float a  = i * 6.0f * DEG_TO_RAD;
        const float sa = sinf(a), ca = cosf(a);
        const bool  hr = (i % 5 == 0);
        const int16_t ir = tickOuter - (hr ? 22 : 10);

        const float ox = FACE_CX + tickOuter * sa, oy = FACE_CY - tickOuter * ca;
        const float ix = FACE_CX + ir * sa,        iy = FACE_CY - ir * ca;

        if (hr)
            tft.drawWideLine(ox, oy, ix, iy, 3.5f, C_TICK_H, C_FACE_BG);
        else
            tft.drawLine((int16_t)ox, (int16_t)oy, (int16_t)ix, (int16_t)iy, C_TICK_M);
    }

    // ── Hour numerals ─────────────────────────────────────────────────────
    for (int i = 0; i < 12; i++) _drawNumeral(i);
}

static void _drawAnalogFace(int hour, int minute) {
    const float ha = ((hour % 12) * 30.0f + minute * 0.5f) * DEG_TO_RAD;
    const float ma = minute * 6.0f * DEG_TO_RAD;

    if (isnan(_prevHa)) {
        // First draw — full redraw of static elements
        _drawFaceStaticElements();
        _moveHand(NAN, ha, HOUR_L, 8.0f);
        _moveHand(NAN, ma, MIN_L,  5.0f);
    } else {
        // Subsequent updates — erase old arms, draw new arms
        _moveHand(_prevHa, ha, HOUR_L, 8.0f);

        // Minute hand: erase old, restore any numeral under the OLD arm, then draw new
        _eraseHand(_prevMa, MIN_L, 5.0f);
        const int oldRem = _prevMin % 5;
        if (oldRem <= 1 || oldRem == 4) {
            const int markIdx = (oldRem == 4) ? ((_prevMin / 5 + 1) % 12)
                                              : ((_prevMin / 5)     % 12);
            _drawNumeral(markIdx);
        }
        _drawHand(ma, MIN_L, 5.0f);
    }

    // Centre pin on top of both arms
    tft.fillCircle(FACE_CX, FACE_CY, 8, C_CENTER_DOT);

    _prevHa  = ha;
    _prevMa  = ma;
    _prevMin = minute;
}

// Draw the static outer ring (called once per style change).
static void _setupAnalog() {
    _prevHa = NAN;  // force full face redraw on next drawFastClock call
    _prevMa = NAN;
    tft.fillCircle(FACE_CX, FACE_CY, FACE_R,     C_RING);
    tft.fillCircle(FACE_CX, FACE_CY, FACE_R - 3, C_FACE_BG);
    _drawAnalogFace(0, 0);
}

// ===========================================================================
//  12/24-hour helpers
// ===========================================================================

// Convert a 24h hour (0-23) to 12h display hour (1-12).
static int _to12h(int h24) {
    int h = h24 % 12;
    return (h == 0) ? 12 : h;
}

// AM/PM indicator — drawn in the upper-left corner of the clock area.
// Erased by writing background colour over the same region.
#define AMPM_X   4
#define AMPM_Y   4
#define AMPM_FONT 6   // 32×64 px characters (16×32 CGROM × 2× scale) → "AM"/"PM" = ~70 px wide

static void _drawAmPm(int hour24) {
    const char* label = (hour24 < 12) ? "AM" : "PM";
    tft.setTextColor(C_BTN_TXT, C_BG);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(label, AMPM_X, AMPM_Y, AMPM_FONT);
}

static void _eraseAmPm() {
    // Clear the region used by the AM/PM label (font6 = 32 px wide × 64 px high per char)
    tft.fillRect(AMPM_X, AMPM_Y, 72, 64, C_BG);
}

// ===========================================================================
//  Public API
// ===========================================================================

void setupDisplay() {
    DISPLAY_SPI.setTX(DISPLAY_SDO);   // gp11 — SPI1 TX → display SDI
    DISPLAY_SPI.setRX(DISPLAY_SDI);   // gp8  — SPI1 RX ← display SDO
    DISPLAY_SPI.setSCK(DISPLAY_SCK);
    DISPLAY_SPI.begin(false);         // manual CS

#ifdef TOUCH_CS
    pinMode(TOUCH_CS, OUTPUT);  digitalWrite(TOUCH_CS, HIGH);
    Serial.print(F("Touch CS=gp")); Serial.print(TOUCH_CS);
    Serial.print(F(" MISO=gp"));   Serial.println(TOUCH_MISO);
#else
    Serial.println(F("Touch: TOUCH_CS not defined — touch disabled"));
#endif

    if (!tft.init()) {
        Serial.println("LT7381/RA8876 display init failed");
        return;
    }
    tft.setRotation(ROTATION);
    tft.fillScreen(C_BG);

    // Restore last-used style and 12h setting from RP2040 internal flash EEPROM
    EEPROM.begin(4);
    uint8_t saved = EEPROM.read(STYLE_EEPROM_ADDR);
    uint8_t saved12 = EEPROM.read(HOUR12_EEPROM_ADDR);
    EEPROM.end();
    if (saved < STYLE_COUNT) _style = (ClockStyle)saved;
    _hour12 = (saved12 == 1);

    _computeButtons();  // must come after _style is restored

    if      (_style == STYLE_ANALOG)    _setupAnalog();
    else if (_style == STYLE_NIXIE)     _setupNixie();
    else if (_style == STYLE_PRAGOTRON) _setupPragotron();
    else if (_style == STYLE_FLIP)      _setupFlip();
    else                                _setupDigital();

    _drawButtons();
    if (_hour12) _drawAmPm(_lastHour);

    const char* styleNames[] = { "analog", "digital", "nixie", "pragotron", "flip" };
    Serial.printf("Display ready (%s style, %s)\n", styleNames[_style], _hour12 ? "12h" : "24h");
}

// ---------------------------------------------------------------------------
void drawFastClock(int hour, int minute) {
    _lastHour   = hour;
    _lastMinute = minute;
    const int dispHour = _hour12 ? _to12h(hour) : hour;
    if      (_style == STYLE_ANALOG)    _drawAnalogFace(dispHour, minute);
    else if (_style == STYLE_NIXIE)     _updateNixie(dispHour, minute);
    else if (_style == STYLE_PRAGOTRON) _updatePragotron(dispHour, minute);
    else if (_style == STYLE_FLIP)      _updateFlip(dispHour, minute);
    else                                _updateDigital(dispHour, minute);
    if (_hour12) _drawAmPm(hour);
}

// ---------------------------------------------------------------------------
//  updateClockStatus — call with clock->state.rate.rate on every time event.
//  Redraws the Run/Pause button only when the running state changes.
// ---------------------------------------------------------------------------
void updateClockStatus(float rate) {
    const bool nowRunning = (rate > 0.0f);
    if (nowRunning != _running) {
        _running = nowRunning;
        _drawOneButton(2);  // RUN/PAUSE is now button index 2
    }
}

// ---------------------------------------------------------------------------
//  setClockStyle — called by the Style button touch handler (Phase 3).
//  Clears the screen and redraws for the new style.
// ---------------------------------------------------------------------------
void setClockStyle(ClockStyle s) {
    _style = s;
    tft.fillScreen(C_BG);
    _computeButtons();
    for (int i = 0; i < 4; i++) _prev[i] = -1;  // force full digit redraw

    if      (_style == STYLE_ANALOG)    _setupAnalog();
    else if (_style == STYLE_NIXIE)     _setupNixie();
    else if (_style == STYLE_PRAGOTRON) _setupPragotron();
    else if (_style == STYLE_FLIP)      _setupFlip();
    else                                _setupDigital();

    _drawButtons();

    // Persist the new style to RP2040 internal flash EEPROM
    EEPROM.begin(4);
    EEPROM.write(STYLE_EEPROM_ADDR, (uint8_t)s);
    EEPROM.commit();
    EEPROM.end();

    // Redraw immediately at the last known time — no need to wait for LCC event.
    drawFastClock(_lastHour, _lastMinute);  // also redraws AM/PM if needed
}

ClockStyle getClockStyle() { return _style; }

bool is12HourMode() { return _hour12; }

void toggle12HourMode() {
    _hour12 = !_hour12;

    // Persist to EEPROM
    EEPROM.begin(4);
    EEPROM.write(HOUR12_EEPROM_ADDR, _hour12 ? 1 : 0);
    EEPROM.commit();
    EEPROM.end();

    // Erase or draw AM/PM label, then redraw clock with updated hour format
    if (!_hour12) _eraseAmPm();
    // Force digit re-render by resetting change tracking
    for (int i = 0; i < 4; i++) _prev[i] = -1;
    _prevHa = NAN;
    _prevMa = NAN;
    // Full redraw
    tft.fillScreen(C_BG);
    _computeButtons();
    if      (_style == STYLE_ANALOG)    _setupAnalog();
    else if (_style == STYLE_NIXIE)     _setupNixie();
    else if (_style == STYLE_PRAGOTRON) _setupPragotron();
    else if (_style == STYLE_FLIP)      _setupFlip();
    else                                _setupDigital();
    _drawButtons();
    drawFastClock(_lastHour, _lastMinute);

    Serial.printf("12h mode: %s\n", _hour12 ? "ON" : "OFF");
}

// ===========================================================================
//  XPT2046 resistive touch — hardware SPI1, shared MISO with RA8876
//
//  On this display module D_SDO (RA8876 MISO) and RTP_DOUT (XPT2046 DOUT)
//  are on the same net, so no MISO pin switching is needed.  CS lines
//  (DISPLAY_CS gp9, TOUCH_CS gp13) select the active chip.  SPI mode is
//  switched via beginTransaction: Mode 3 for RA8876, Mode 0 for XPT2046.
//
//  Touch detection uses Z1 pressure (no PENIRQ pin required).
//  Z1 ~ 0 when not touched; increases when screen is pressed.
//
//  Calibration constants — tune after first successful read:
// ===========================================================================
#ifdef TOUCH_CS

#define TOUCH_Z_THRESHOLD   50   // Z1 above this = touched (0 = not touched)
#define TOUCH_RAW_X_MIN     150  // raw X at left edge   (observed ~195)
#define TOUCH_RAW_X_MAX    1950  // raw X at right edge  (observed ~1916)
#define TOUCH_RAW_Y_MIN      50  // raw Y at top edge    (observed ~87)
#define TOUCH_RAW_Y_MAX    1980  // raw Y at bottom edge (observed ~1960)
#define TOUCH_DEBOUNCE_MS    60

// Read one XPT2046 channel via hardware SPI.
// Uses 3 transfers: command byte (discard), then two data bytes for 12-bit result.
// XPT2046 outputs: [busy][D11..D5] in byte 2, [D4..D0][0..0] in byte 3.
// SPI1 must be in an active beginTransaction() when called.
static int16_t _xptRead(uint8_t cmd) {
    DISPLAY_SPI.transfer(cmd);                   // send command; discard response
    uint8_t hi = DISPLAY_SPI.transfer(0x00);     // D11..D4
    uint8_t lo = DISPLAY_SPI.transfer(0x00);     // D3..D0 in upper nibble
    return (int16_t)((hi << 4) | (lo >> 4));     // assemble 12-bit result
}

// Read XPT2046 touch X/Y/Z1 via SPI1.
// No MISO switching needed: D_SDO (gp8) and RTP_DOUT (gp12) are on the same
// net on this display module, so gp8 serves both the RA8876 and XPT2046.
// We simply switch CS and use Mode 0 (XPT2046) vs Mode 3 (RA8876) via
// beginTransaction — no SPI.end()/begin() required.
// Returns true if Z1 pressure indicates a touch.
static bool _xptReadXY(int16_t &rawX, int16_t &rawY) {
    digitalWrite(DISPLAY_CS, HIGH);  // RA8876 deasserted

    // beginTransaction BEFORE asserting CS — ensures clock is at correct idle
    // state (Mode 3 = HIGH) before XPT2046 sees CS go LOW.  XPT2046 supports
    // both Mode 0 and Mode 3; staying in Mode 3 avoids clock glitches when
    // transitioning from display transactions.
    DISPLAY_SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE3));

    digitalWrite(TOUCH_CS, LOW);     // XPT2046 selected

    // Read Z1 pressure (PD=11 keep powered)
    int16_t z1 = _xptRead(0xB3);

    bool touched = (z1 > TOUCH_Z_THRESHOLD);
    if (touched) {
        rawX = _xptRead(0x93);  // X-axis differential, PD=11
        rawY = _xptRead(0xD3);  // Y-axis differential, PD=11
    }

    digitalWrite(TOUCH_CS, HIGH);
    DISPLAY_SPI.endTransaction();

    return touched;
}

static bool _touchToScreen(int16_t rawX, int16_t rawY, int16_t &scrX, int16_t &scrY) {
    // The touch panel is rotated 90° CW relative to the display:
    //   rawX (physical horizontal axis) maps to display scrY (vertical)
    //   rawY (physical vertical axis)   maps to display scrX (horizontal)
    if (rawX < TOUCH_RAW_X_MIN || rawX > TOUCH_RAW_X_MAX) return false;
    if (rawY < TOUCH_RAW_Y_MIN || rawY > TOUCH_RAW_Y_MAX) return false;
    scrX = map(rawY, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX, 0, HRES - 1);
    scrY = map(rawX, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX, 0, VRES - 1);
    return true;
}

static uint32_t _touchDownMs  = 0;
static bool     _touchPending = false;
static bool     _touchHandled = false;

void processTouchDisplay() {
    int16_t rawX = 0, rawY = 0;
    bool touched = _xptReadXY(rawX, rawY);
    if (!touched) { _touchPending = false; return; }

    int16_t scrX, scrY;
    if (!_touchToScreen(rawX, rawY, scrX, scrY)) return;

    // Debug: print mapped screen coords on every new touch-down
    static int16_t _lastScrX = -1, _lastScrY = -1;
    if (!_touchPending) {
        Serial.printf("Touch scrX=%d scrY=%d\n", scrX, scrY);
        _lastScrX = scrX; _lastScrY = scrY;
    }

    if (!_touchPending) {
        _touchPending = true;
        _touchHandled = false;
        _touchDownMs  = millis();
    }
    if (!_touchHandled && (millis() - _touchDownMs >= TOUCH_DEBOUNCE_MS)) {
        _touchHandled = true;
        Serial.printf("HitTest: style=%d btn[0]={%d,%d} scrX=%d scrY=%d\n",
                      (int)_style, _btns[0].x, _btns[0].y, scrX, scrY);
        bool hit = false;

        // Top-left corner (80×80 px) — toggle 12/24h format
        if (scrX < 80 && scrY < 80) {
            hit = true;
            Serial.println(F("Touch: top-left corner — toggle 12/24h"));
            toggle12HourMode();
        }

        if (!hit) {
            for (int i = 0; i < 4; i++) {
                const ButtonRect& b = _btns[i];
                if (scrX >= b.x && scrX < b.x + b.w &&
                    scrY >= b.y && scrY < b.y + b.h) {
                    hit = true;
                    switch (i) {
                        case 0: Serial.println(F("Touch: BTN A"));     break;
                        case 1: Serial.println(F("Touch: BTN B"));     break;
                        case 2: Serial.println(F("Touch: RUN/PAUSE")); break;
                        case 3: Serial.println(F("Touch: STYLE")); setClockStyle((ClockStyle)(((int)getClockStyle() + 1) % STYLE_COUNT)); break;
                    }
                    break;
                }
            }
        }
        if (!hit) Serial.printf("Touch: no button hit at scrX=%d scrY=%d\n", scrX, scrY);
    }
}

#else
void processTouchDisplay() {}
#endif  // TOUCH_CS

// ===========================================================================
//  No-op stubs for boards without a display
// ===========================================================================
#else

void       setupDisplay()                  {}
void       drawFastClock(int, int)         {}
void       updateClockStatus(float)        {}
void       setClockStyle(ClockStyle)       {}
ClockStyle getClockStyle()                 { return STYLE_ANALOG; }
void       toggle12HourMode()              {}
bool       is12HourMode()                  { return false; }
void       processTouchDisplay()           {}

#endif  // DISPLAY_DRIVER_RA8876_NATIVE && DISPLAY_CS
