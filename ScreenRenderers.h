// =====================================================================================================================
// --- Screen Renderers (ScreenRenderers.h) ---
// =====================================================================================================================
#ifndef SCREEN_RENDERERS_H
#define SCREEN_RENDERERS_H

#include "Config.h"
#include "StateManager.h"
#include "FuelMonitor.h"
#include "EngineMonitor.h"
#include "DisplayComponents.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

void render_mode_0_screen(Adafruit_GFX* oled_obj, const Mode0DisplayData& data);
void render_mode_1_screen(Adafruit_GFX* oled_obj, const Mode1DisplayData& data);
void render_mode_2_screen(Adafruit_GFX* oled_obj, const Mode2DisplayData& data);
void render_service_mode_screen(Adafruit_GFX* oled_obj, const ServiceModeDisplayData& data);

#endif // SCREEN_RENDERERS_H
