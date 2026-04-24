// =====================================================================================================================
// --- Display Extensions (DisplayExtensions.h) ---
// =====================================================================================================================
#ifndef DISPLAY_EXTENSIONS_H
#define DISPLAY_EXTENSIONS_H

#include <Adafruit_GFX.h>

void stretched_text(Adafruit_GFX* display, const char* text, int16_t x, int16_t y, uint8_t size_x, uint8_t size_y, uint16_t color);
void large_text(Adafruit_GFX* display, const char* text, int16_t x, int16_t y, uint8_t size, uint16_t color);

#endif // DISPLAY_EXTENSIONS_H
