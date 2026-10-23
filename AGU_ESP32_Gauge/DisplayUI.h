#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <TFT_eSPI.h>
#include "Config.h"

class DisplayUI {
public:
    DisplayUI();
    void begin();
    void update(const SensorData &data);
    void nextTheme(SensorData &data);
    void checkTouch(SensorData &data);

private:
    TFT_eSPI tft;
    DisplayTheme lastRenderedTheme;
    unsigned long touchPressStart;
    bool wasTouched;

    // Farbpalette
    const uint16_t COLOR_BG_DARK       = 0x0821;
    const uint16_t COLOR_OEM_BLUE      = 0x0012; // VW Golf 4 Tacho-Tiefblau
    const uint16_t COLOR_OEM_RED       = 0xF800; // VW Signalrot
    const uint16_t COLOR_GRID_BORDER   = 0x2124; // Dezente Kachelumrandung
    const uint16_t COLOR_ACCENT_CYAN   = 0x07FF; // Cyan für Ladedruck
    const uint16_t COLOR_ACCENT_ORANGE = 0xFD20; // Orange für EGT / Öldruck
    const uint16_t COLOR_ACCENT_GREEN  = 0x07E0; // Grün für Normalwerte
    const uint16_t COLOR_ACCENT_YELLOW = 0xFFE0; // Gelb für Warnung
    const uint16_t COLOR_COLD_BLUE     = 0x3CF;  // Kaltes Öl (< 75°C)

    void initTheme(DisplayTheme theme);

    // Die 4 motorzustands-orientierten Design-Studien
    void renderHealthGrid(const SensorData &data);
    void renderPressureTemp(const SensorData &data);
    void renderOemVdo(const SensorData &data);
    void renderMonitorMatrix(const SensorData &data);

    // Hilfsfunktionen
    void drawHealthTile(int x, int y, int w, int h, const char *label, const char *valStr, const char *unit, const char *subtext, uint16_t valColor);
    void drawVdoDial(int cx, int cy, int r, float val, float minV, float maxV, const char *unit, const char *title, uint16_t needleCol);
    void drawHorizontalBar(int x, int y, int w, int h, float val, float minV, float maxV, uint16_t barCol);
};

#endif // DISPLAY_UI_H
