// =====================================================================================================================
// --- Display Extensions Implementation (DisplayExtensions.cpp) ---
// =====================================================================================================================
#include "DisplayExtensions.h"
#include <Adafruit_GFX.h>

void stretched_text(Adafruit_GFX* display, const char* text, int16_t x, int16_t y, uint8_t size_x, uint8_t size_y, uint16_t color /*= 1*/) {
    if (!display || !text || text[0] == '\0') {
        return;
    }
    display->setTextSize(size_x, size_y);
    display->setTextColor(color);
    display->setCursor(x, y);
    display->print(text);
    display->setTextSize(1);
}

void large_text(Adafruit_GFX* display, const char* text, int16_t x, int16_t y, uint8_t size, uint16_t color /*= 1*/) {
    stretched_text(display, text, x, y, size, size, color);
}
