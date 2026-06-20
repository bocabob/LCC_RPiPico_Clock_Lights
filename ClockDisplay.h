#pragma once

// ---------------------------------------------------------------------------
//  Clock display styles — cycled by the Style button (Phase 3: touch)
// ---------------------------------------------------------------------------
enum ClockStyle {
    STYLE_ANALOG    = 0,  // analog face, button column on right
    STYLE_DIGITAL   = 1,  // 7-segment HH:MM, button bar on bottom
    STYLE_NIXIE     = 2,  // glowing nixie-tube 7-segment, button bar on bottom
    STYLE_PRAGOTRON = 3,  // split-flap tile digits (JMRI images), button bar on bottom
    STYLE_FLIP      = 4,  // stroke-based flip clock (drawn tiles), button bar on bottom
    STYLE_COUNT     = 5   // sentinel; add future styles before COUNT
};

// ---------------------------------------------------------------------------
//  Public API
// ---------------------------------------------------------------------------
void       setupDisplay();                       // call once from setup()
void       drawFastClock(int hour, int minute);  // call on LCC time-changed event
void       updateClockStatus(float rate);        // call on LCC time-changed event (rate ≥ 0)
void       setClockStyle(ClockStyle style);      // cycle styles; call from touch handler
ClockStyle getClockStyle();
void       toggle12HourMode();                   // toggle 12/24h format; persisted to EEPROM
bool       is12HourMode();
void       processTouchDisplay();                // call from loop() — stub until touch wired
