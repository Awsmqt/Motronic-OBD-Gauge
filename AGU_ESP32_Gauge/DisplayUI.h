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
    const uint16_t COLOR_BG_DARK       = 0x0821; // Sehr dunkles Grau/Blau
    const uint16_t COLOR_OEM_BLUE      = 0x0014; // VW Golf 4 Tacho-Tiefblau
    const uint16_t COLOR_OEM_RED       = 0xF800; // VW Signalrot
    const uint16_t COLOR_GRID_BORDER   = 0x31A6; // Dezente Kachelumrandung
    const uint16_t COLOR_ACCENT_CYAN   = 0x07FF; // Cyan für Ladedruck
    const uint16_t COLOR_ACCENT_ORANGE = 0xFD20; // Orange für EGT
    const uint16_t COLOR_ACCENT_GREEN  = 0x07E0; // Grün für Normalwerte
    const uint16_t COLOR_ACCENT_YELLOW = 0xFFE0; // Gelb für Warnung

    // Layout Initialisierungen
    void initTheme(DisplayTheme theme);
    void renderHeader(const SensorData &data, const char *title);
    void renderFooter(const SensorData &data);

    // Die 4 Design-Studien
    void renderClassicQuad(const SensorData &data);
    void renderSportCluster(const SensorData &data);
    void renderOemPlus(const SensorData &data);
    void renderDataMatrix(const SensorData &data);

    // Hilfsfunktionen für Grafik-Elemente
    void drawGaugeArc(int cx, int cy, int radius, float value, float minVal, float maxVal, uint16_t color);
    void drawRpmShiftBar(int x, int y, int w, int h, int rpm, int maxRpm);
    void drawStatBox(int x, int y, int w, int h, const char *label, const char *valStr, const char *unit, uint16_t valColor);
};

#endif // DISPLAY_UI_H
